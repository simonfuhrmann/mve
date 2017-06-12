
#include <cstring>
#include <cstdlib>


#include <iostream>
#include <fstream>
#include <ostream>
#include <vector>
#include <string>




#include "math/matrix.h"

#include "mve/camera.h"

#include "mve/image.h"
#include "mve/image_tools.h"
#include "mve/image_io.h"
#include "mve/image_exif.h"

#include "mve/mesh_info.h"
#include "mve/mesh_io.h"
#include "mve/mesh_io_ply.h"
#include "mve/mesh_tools.h"

#include "mve/scene.h"
#include "mve/view.h"
#include "mve/volume.h"

#include "util/file_system.h"
#include "util/string.h"
#include "util/system.h"


/* ---------------------------------------------------------------- */

mve::ByteImage::Ptr
load_8bit_image (std::string const& fname, std::string* exif)
{
    std::string lcfname(util::string::lowercase(fname));
    std::string ext4 = util::string::right(lcfname, 4);
    std::string ext5 = util::string::right(lcfname, 5);
    try
    {
        if (ext4 == ".jpg" || ext5 == ".jpeg")
            return mve::image::load_jpg_file(fname, exif);
        else if (ext4 == ".png" ||  ext4 == ".ppm"
            || ext4 == ".tif" || ext5 == ".tiff")
            return mve::image::load_file(fname);
    }
    catch (...)
    {
        throw;
    }

    return mve::ByteImage::Ptr();
}
/* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- */
std::string
remove_file_extension (std::string const& filename)
{
    std::size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos)
        return filename.substr(0, pos);
    return filename;
}
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

std::string
make_image_name (int id)
{
    return "view_" + util::string::get_filled(id, 4) + ".mve";
}

/* ---------------------------------------------------------------- */

std::string
load_precomputed_camera_info(std::string const& filename)
{
    std::string camera_info;
    util::fs::read_file_to_string(filename, &camera_info);

    return camera_info;
}

/*------------------------------------------------------------------*/

math::Matrix4f load_matrice(char* path)
{
	math::Matrix4f projection_matrix;
	std::ifstream filep(path,std::ifstream::in);

	        if(!filep)
	        {
	            std::cerr << " unable to open file!!!"<<std::endl;
	            exit(EXIT_FAILURE);
	        }
	        for(int i = 0; i < 3 ; i++)
	        {

	            for(int j = 0; j < 4; j++)
	            {
	            	filep>>projection_matrix(i,j);
	            }
	        }
	        filep.close();

	        projection_matrix(3,0) =  0.0;
	        projection_matrix(3,1) =  0.0;
	        projection_matrix(3,2) =  0.0;
	        projection_matrix(3,3) =  1.0;


	return projection_matrix;
}

/*.................................................................................*/

mve::ByteImage::Ptr process_image(mve::ByteImage::Ptr img)
{
	mve::ByteImage::Ptr out = img->duplicate();
	for(int x = 0; x < out->width() ; x++)
	{
		for(int y = 0; y < out->height(); y++)
		{
			 int gray = 0.299 * img->at(x,y,0) + 0.587 * img->at(x,y,1) + 0.114 * img->at(x,y,2); /* Object_2*/

			if(gray >= 20)								 /*if(img->at(x,y,0) > img->at(x,y,2))*/ /*Object_1*/
			{
				out->at(x,y,0) =  255;
				out->at(x,y,1) =  255;
				out->at(x,y,2) =  255;
			}
			else
			{
				out->at(x,y,0) =  0;
				out->at(x,y,1) =  0;
				out->at(x,y,2) =  0;
			}
		}
	}


	return out;

}
/*...........................................................................*/
bool check(int u, int v,int h, int w, mve::ByteImage::Ptr img)
{
	if((u>= 0  && u < h) && (v >=0 && v< w) )
	 {
		  if(img->at(u,v,0) == 255)
		  {
			  return true;
		  }
	  }

	  return false;
}

/*...........................................................................*/

struct Element		// data structure for volume
{
	math::Vec4f pos;
	math::Vec4f color;
};

int main()
{

	std::vector<math::Matrix4f> projection;
	/* 1- let's make views from input images */

    std::string input_path  = "data/Object_2/visualize";
    std::string camera_path = "data/Object_2/projection/%d.txt";
    std::string output_path = "Scene Output/";

    util::fs::Directory dir;	/* Directory is a class inherited from std::vector<file> it */
    							/* is used to scan a given directory  contents */

    try
     	 {
    		dir.scan(input_path);
     	 }
    catch (std::exception& e)	/* find any system problem while scanning the directory*/
    	{
    		std::cerr << " Error while scanning the input directory: "<< e.what()<<std::endl;
    		 std::exit(EXIT_FAILURE);
    	}

    std::cout<< dir.size() <<" entries is found"<<std::endl;


    /* create output directory*/
    std::string views_path = util::fs::join_path(output_path,"views/");

    std::cout << "Creating Output directories... "<<std::endl;
    util::fs::mkdir(output_path.c_str());
    util::fs::mkdir(views_path.c_str());

    /* sort the input directory and loop over it */
    std::sort(dir.begin(),dir.end());

    projection.resize(dir.size());

    for(std::size_t i = 0; i < dir.size(); ++i)
    {
    	std::string filename = dir[i].name;
    	std::string absolutefilename = dir[i].get_absolute_name();
    	std::string exif;

    	/* load only 8 bit images */
    	mve::ByteImage::Ptr origine = load_8bit_image(absolutefilename, &exif);
        if (origine == nullptr)
            continue;
        mve::ByteImage::Ptr image = process_image(origine); /* create the silhouette */

        /* create view set all meta information */
        mve::View::Ptr view = mve::View::create();
        view->set_id(i);
        view->set_name(remove_file_extension(filename));

        // TODO add camera info to the meta data.. right now i can't i don't know how
        /*........Improvising .....*/
        char* fName = new char[255];
        sprintf(fName,camera_path.c_str(),i);
        projection[i] = load_matrice(fName);
        delete[] fName;

        /* add image to the view */
        view->set_image(image, "original");

        /* Save view to disc */
        std::string mve_fname = make_image_name(i);

        std::cout << "Importing image: " << filename
            << ", writing MVE view: " << mve_fname << "..." << std::endl;
        view->save_view_as(util::fs::join_path(views_path, mve_fname));
    }

    /* Load scene */
    mve::Scene::Ptr scene = mve::Scene::create();
    scene->load_scene("Scene Output/");

    mve::Scene::ViewList const& views = scene->get_views();


    /* -3 create the volume */

    std::cout<< "Creating volume...";
    mve::Volume<Element>::Ptr grid = mve::Volume<Element>::create(200,200,200);
    mve::Volume<Element>::Voxels &data = grid->get_data();


    /* float resolution = pow(0.014928/8000000.0,1.0/3.0); */ /* Object_1*/
    float resolution = pow(0.0048640/8000000.0,1.0/3.0);       /*Object_2*/
    int indx = 0;
  	for(int i = 0; i < 200; i ++)
  	    {
  	        for(int j = 0; j < 200; j ++)
  	        {
  	            for(int k =0 ; k < 200; k++)
  	            {
  	              Element voxel;
  	              /* voxel.pos   = math::Vec4f(-0.13066  + i * resolution, -0.13066 + j * resolution,  -0.74832  + k * resolution,1.0); */ /* Object_1 */
  	              voxel.pos   = math::Vec4f(-0.05  + i * resolution, -0.022 + j * resolution,  -0.1  + k * resolution,1.0);  			   /* Object_2*/
  	              voxel.color = math::Vec4f(1.0,1.0,1.0,1.0);
  	              data[indx] = voxel;
  	              indx++;
  	            }
  	        }
  	    }

  	std::cout<< "Done."<<std::endl;

    /* 4 calculate the visual hull */
    std::cout<< "Reconstructing visual hull...";
    bool keep;
    for(std::size_t i = 0; i < data.size() ; ++i)
    {
    	  math::Vec4f pos = data[i].pos;
    	  for (std::size_t p = 0; p < views.size(); ++p)
    	   {

    		  math::Matrix4f mat =  projection[p];	/* the pre-calculated projection matrix */
    		  math::Vec4f screen =  mat * pos;

    		  int u = (int) ( screen[0] / screen[2] );
    		  int v = (int) ( screen[1] / screen[2] );


    		  mve::ByteImage::Ptr silhouette = views[p]->get_byte_image("original");

    		  int h = silhouette->height();
    		  int w = silhouette->width();

    		  if(check(u,v,w,h,silhouette) == true)
    		  {
    			  keep = true;
    		  }
    		  else
    		  {
				  keep = false;
				  break;
    		  }
    	   }

    	  if(keep == false)
    		  data[i].color[0] = 0.0;
    }
    std::cout<< "Done."<< std::endl;

    /* 5 save the volume into ply file */
    std::cout<< "saving to PLY...";
    /* Prepare output mesh. */
    mve::TriangleMesh::Ptr pset = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = pset->get_vertices();
    mve::TriangleMesh::ColorList& vcolor = pset->get_vertex_colors();


    for(std::size_t i = 0; i < data.size() ; ++i)
    {
    	math::Vec3f v = math::Vec3f(data[i].pos[0],data[i].pos[1],data[i].pos[2]);
    	math::Vec4f c = data[i].color;	/* in an other version of this sample i will try to estimate colors for know it is constant white */

    	if(c[0] == 1.0)
    	{
    		verts.push_back(v);
    		vcolor.push_back(c);
    	}

    }


    /* write to hdd */
    mve::geom::SavePLYOptions opts;
    opts.write_vertex_normals = false;
    opts.write_vertex_values =  false;
    opts.write_vertex_confidences = false;
    mve::geom::save_ply_mesh(pset, "cloud.ply", opts);



	return 0;
}
