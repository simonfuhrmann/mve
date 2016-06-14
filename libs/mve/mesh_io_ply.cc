/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <cerrno>

#include "util/exception.h"
#include "util/tokenizer.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "mve/depthmap.h"
#include "mve/mesh_io_ply.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

template <typename T>
T
ply_read_value (std::istream& input, PLYFormat format)
{
    T value;
    switch (format)
    {
        case PLY_ASCII:
            input >> value;
            return value;

        case PLY_BINARY_LE:
            input.read(reinterpret_cast<char*>(&value), sizeof(T));
            return util::system::letoh(value);

        case PLY_BINARY_BE:
            input.read(reinterpret_cast<char*>(&value), sizeof(T));
            return util::system::betoh(value);

        default:
            throw std::invalid_argument("Invalid data format");
    }
}

/*
 * Template specialization for 'unsigned char', as the generic template
 * function will handle it incorrectly (reads one char, not the number).
 */
template <>
unsigned char
ply_read_value<unsigned char> (std::istream& input, PLYFormat format)
{
    switch (format)
    {
        case PLY_ASCII:
        {
            int temp;
            input >> temp;
            return static_cast<unsigned char>(temp);
        }

        case PLY_BINARY_LE:
        case PLY_BINARY_BE:
        {
            unsigned char value;
            input.read(reinterpret_cast<char*>(&value), 1);
            return value;
        }

        default:
            throw std::invalid_argument("Invalid data format");
    }
}

/* Declare specialization for 'char' but leave it undefined for now. */
template <>
char
ply_read_value<char> (std::istream& input, PLYFormat format);

/* Explicit template instantiation for 'float'. */
template
float
ply_read_value<float> (std::istream& input, PLYFormat format);

/* Explicit template instantiation for 'double'. */
template
double
ply_read_value<double> (std::istream& input, PLYFormat format);

/* Explicit template instantiation for 'unsigned int'. */
template
unsigned int
ply_read_value<unsigned int> (std::istream& input, PLYFormat format);

/* Explicit template instantiation for 'int'. */
template
int
ply_read_value<int> (std::istream& input, PLYFormat format);

/* ---------------------------------------------------------------- */

/* Converts 'num' float values in [0,1] to unsigned char in [0,255]. */
void
ply_color_convert (float const* src, unsigned char* dest, int num = 3)
{
    for (int c = 0; c < num; ++c)
    {
        float color = src[c];
        color = color * 255.0f;
        color = std::min(255.0f, std::max(0.0f, color));
        dest[c] = (unsigned char)(color + 0.5f);
    }
}

/* ---------------------------------------------------------------- */
// TODO check token amount to prevent undefined access

TriangleMesh::Ptr
load_ply_mesh (std::string const& filename)
{
    /* Precondition checks. */
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    /* Open file. */
    std::ifstream input(filename.c_str(), std::ios::binary);
    if (!input.good())
        throw util::FileException(filename, std::strerror(errno));

    /* Start parsing. */
    std::string buffer;
    input >> buffer; /* Read "ply" file signature. */
    if (buffer != "ply")
    {
        input.close();
        throw util::Exception("File format not recognized as PLY-model");
    }

    /* Discard the rest of the line. */
    std::getline(input, buffer);

    PLYFormat ply_format = PLY_UNKNOWN;
    std::size_t num_faces = 0;
    std::size_t num_vertices = 0;
    std::size_t num_grid = 0;
    std::size_t num_tristrips = 0;
    std::size_t grid_cols = 0;
    std::size_t grid_rows = 0;
    std::vector<PLYVertexProperty> v_format;
    std::vector<PLYFaceProperty> f_format;

    bool critical = false;
    bool reading_verts = false;
    bool reading_faces = false;
    bool reading_grid = false;
    bool reading_tristrips = false;
    std::size_t skip_bytes = 0;

    while (input.good())
    {
        std::getline(input, buffer);
        util::string::clip_newlines(&buffer);
        util::string::clip_whitespaces(&buffer);

        //std::cout << "Buffer: " << buffer << std::endl;

        if (buffer == "end_header")
            break;

        util::Tokenizer header;
        header.split(buffer);

        if (header.empty())
            continue;

        if (header[0] == "format")
        {
            /* Determine the format. */
            if (header[1] == "ascii")
                ply_format = PLY_ASCII;
            else if (header[1] == "binary_little_endian")
                ply_format = PLY_BINARY_LE;
            else if (header[1] == "binary_big_endian")
                ply_format = PLY_BINARY_BE;
            else
                ply_format = PLY_UNKNOWN;
        }
        else if (header[0] == "comment")
        {
            /* Print all comments to STDOUT and forget data. */
            std::cout << "PLY Loader: " << buffer << std::endl;
        }
        else if (header[0] == "element")
        {
            reading_faces = false;
            reading_verts = false;
            reading_grid = false;
            reading_tristrips = false;
            if (header[1] == "vertex")
            {
                reading_verts = true;
                num_vertices = util::string::convert<std::size_t>(header[2]);
            }
            else if (header[1] == "face")
            {
                reading_faces = true;
                num_faces = util::string::convert<std::size_t>(header[2]);
            }
            else if (header[1] == "range_grid")
            {
                reading_grid = true;
                num_grid = util::string::convert<std::size_t>(header[2]);
            }
            else if (header[1] == "tristrips")
            {
                reading_tristrips = true;
                num_tristrips = util::string::convert<std::size_t>(header[2]);
            }
            else
            {
                std::cout << "PLY Loader: Element \"" << header[1]
                    << "\" not recognized" << std::endl;
            }

            /* Aligned range images in PLY format may have a camera inside. */
            if (header[1] == "camera")
                skip_bytes = 23 * 4;
        }
        else if (header[0] == "obj_info")
        {
            if (header[1] == "num_cols")
                grid_cols = util::string::convert<std::size_t>(header[2]);
            else if (header[1] == "num_rows")
                grid_rows = util::string::convert<std::size_t>(header[2]);
        }
        else if (header[0] == "property")
        {
            if (reading_verts)
            {
                /* List of accepted and handled attributes. */
                if (header[1] == "float" || header[1] == "float32")
                {
                    /* Accept float x,y,z values. */
                    if (header[2] == "x")
                        v_format.push_back(PLY_V_FLOAT_X);
                    else if (header[2] == "y")
                        v_format.push_back(PLY_V_FLOAT_Y);
                    else if (header[2] == "z")
                        v_format.push_back(PLY_V_FLOAT_Z);
                    else if (header[2] == "nx")
                        v_format.push_back(PLY_V_FLOAT_NX);
                    else if (header[2] == "ny")
                        v_format.push_back(PLY_V_FLOAT_NY);
                    else if (header[2] == "nz")
                        v_format.push_back(PLY_V_FLOAT_NZ);
                    else if (header[2] == "r" || header[2] == "red")
                        v_format.push_back(PLY_V_FLOAT_R);
                    else if (header[2] == "g" || header[2] == "green")
                        v_format.push_back(PLY_V_FLOAT_G);
                    else if (header[2] == "b" || header[2] == "blue")
                        v_format.push_back(PLY_V_FLOAT_B);
                    else if (header[2] == "u")
                        v_format.push_back(PLY_V_FLOAT_U);
                    else if (header[2] == "v")
                        v_format.push_back(PLY_V_FLOAT_V);
                    else if (header[2] == "confidence")
                        v_format.push_back(PLY_V_FLOAT_CONF);
                    else if (header[2] == "value")
                        v_format.push_back(PLY_V_FLOAT_VALUE);
                    else
                        v_format.push_back(PLY_V_IGNORE_FLOAT);
                }
                /* List of accepted and handled attributes. */
                else if (header[1] == "double" || header[1] == "float64")
                {
                    /* Accept float x,y,z values. */
                    if (header[2] == "x")
                        v_format.push_back(PLY_V_DOUBLE_X);
                    else if (header[2] == "y")
                        v_format.push_back(PLY_V_DOUBLE_Y);
                    else if (header[2] == "z")
                        v_format.push_back(PLY_V_DOUBLE_Z);
                    else
                        v_format.push_back(PLY_V_IGNORE_DOUBLE);
                }
                else if (header[1] == "uchar" || header[1] == "uint8")
                {
                    /* Accept uchar r,g,b values. */
                    if (header[2] == "r" || header[2] == "red"
                        || header[2] == "diffuse_red")
                        v_format.push_back(PLY_V_UINT8_R);
                    else if (header[2] == "g" || header[2] == "green"
                        || header[2] == "diffuse_green")
                        v_format.push_back(PLY_V_UINT8_G);
                    else if (header[2] == "b" || header[2] == "blue"
                        || header[2] == "diffuse_blue")
                        v_format.push_back(PLY_V_UINT8_B);
                    else
                        v_format.push_back(PLY_V_IGNORE_UINT8);
                }
                else if (header[1] == "int")
                {
                    /* Ignore int data types. */
                    v_format.push_back(PLY_V_IGNORE_UINT32);
                }
                else
                {
                    /* Panic on unhandled data types. */
                    std::cout << "PLY Loader: Unrecognized vertex property \""
                        << buffer << "\"" << std::endl;
                    critical = true;
                    continue;
                }
            }
            else if (reading_faces)
            {
                if (header[1] == "list")
                    f_format.push_back(PLY_F_VERTEX_INDICES);
                else if (header[1] == "int")
                    f_format.push_back(PLY_F_IGNORE_UINT32);
                else if (header[1] == "uchar")
                    f_format.push_back(PLY_F_IGNORE_UINT8);
                else if (header[1] == "float")
                    f_format.push_back(PLY_F_IGNORE_FLOAT);
                else
                {
                    std::cout << "PLY Loader: Unrecognized face property \""
                        << buffer << "\"" << std::endl;
                    critical = true;
                }
            }
            else if (reading_grid)
            {
                if (header[1] != "list")
                {
                    std::cout << "PLY Loader: Unrecognized grid property \""
                        << buffer << "\"" << std::endl;
                    critical = true;
                }
            }
            else if (reading_tristrips)
            {
                if (header[1] != "list")
                {
                    std::cout << "PLY Loader: Unrecognized triangle strips "
                        "property \"" << buffer << "\"" << std::endl;
                    critical = true;
                }
            }
            else
            {
                std::cout << "PLY Loader: Skipping property \""
                    << header[1] << "\" without element." << std::endl;
            }
        }
    }

    if (critical || ply_format == PLY_UNKNOWN)
    {
        input.close();
        throw util::Exception("PLY file encoding not recognized by parser");
    }

    /* Create a new triangle mesh. */
    TriangleMesh::Ptr mesh = TriangleMesh::create();
    TriangleMesh::VertexList& vertices = mesh->get_vertices();
    TriangleMesh::FaceList& faces = mesh->get_faces();
    TriangleMesh::ColorList& vcolors = mesh->get_vertex_colors();
    TriangleMesh::ConfidenceList& vconfs = mesh->get_vertex_confidences();
    TriangleMesh::ValueList& vvalues = mesh->get_vertex_values();
    TriangleMesh::TexCoordList& tcoords = mesh->get_vertex_texcoords();
    TriangleMesh::NormalList& vnormals = mesh->get_vertex_normals();

    bool want_colors = false;
    bool want_vnormals = false;
    bool want_tex_coords = false;

    for (std::size_t i = 0; i < v_format.size(); ++i)
    {
        switch (v_format[i])
        {
            case PLY_V_UINT8_R:
            case PLY_V_UINT8_G:
            case PLY_V_UINT8_B:
            case PLY_V_FLOAT_R:
            case PLY_V_FLOAT_G:
            case PLY_V_FLOAT_B:
                want_colors = true;
                break;

            case PLY_V_FLOAT_NX:
            case PLY_V_FLOAT_NY:
            case PLY_V_FLOAT_NZ:
                want_vnormals = true;
                break;

            case PLY_V_FLOAT_U:
            case PLY_V_FLOAT_V:
                want_tex_coords = true;
                break;

            default:
                break;
        }
    }

    /* Skip some bytes. Usually because of unknown PLY elements. */
    if (skip_bytes > 0)
    {
        std::cout << "PLY Loader: Skipping " << skip_bytes
            << " bytes." << std::endl;
        input.ignore(skip_bytes);
    }

    /* Start reading the vertex data. */
    std::cout << "Reading PLY: " << num_vertices << " verts..." << std::flush;

    vertices.reserve(num_vertices);
    if (want_colors)
        vcolors.reserve(num_vertices);
    if (want_vnormals)
        vnormals.reserve(num_vertices);

    bool eof = false;
    for (std::size_t i = 0; !eof && i < num_vertices; ++i)
    {
        math::Vec3f vertex(0.0f, 0.0f, 0.0f);
        math::Vec3f vnormal(0.0f, 0.0f, 0.0f);
        math::Vec4f color(1.0f, 0.5f, 0.5f, 1.0f); // Make it ugly by default
        math::Vec2f tex_coord(0.0f, 0.0f);

        for (std::size_t n = 0; n < v_format.size(); ++n)
        {
            PLYVertexProperty elem = v_format[n];
            switch (elem)
            {
            case PLY_V_FLOAT_X:
            case PLY_V_FLOAT_Y:
            case PLY_V_FLOAT_Z:
                vertex[(int)elem - PLY_V_FLOAT_X]
                    = ply_read_value<float>(input, ply_format);
                break;

            case PLY_V_DOUBLE_X:
            case PLY_V_DOUBLE_Y:
            case PLY_V_DOUBLE_Z:
                vertex[(int)elem - PLY_V_DOUBLE_X]
                    = ply_read_value<double>(input, ply_format);
                break;

            case PLY_V_FLOAT_NX:
            case PLY_V_FLOAT_NY:
            case PLY_V_FLOAT_NZ:
                vnormal[(int)elem - PLY_V_FLOAT_NX]
                    = ply_read_value<float>(input, ply_format);
                break;

            case PLY_V_UINT8_R:
            case PLY_V_UINT8_G:
            case PLY_V_UINT8_B:
                color[(int)elem - PLY_V_UINT8_R]
                    = (float)ply_read_value<unsigned char>(input, ply_format)
                    * (1.0f / 255.0f);
                break;

            case PLY_V_FLOAT_R:
            case PLY_V_FLOAT_G:
            case PLY_V_FLOAT_B:
                color[(int)elem - PLY_V_FLOAT_R]
                    = ply_read_value<float>(input, ply_format);
                break;

            case PLY_V_FLOAT_U:
            case PLY_V_FLOAT_V:
                tex_coord[(int)elem - PLY_V_FLOAT_U]
                    = ply_read_value<float>(input, ply_format);
                break;

            case PLY_V_FLOAT_CONF:
                vconfs.push_back(ply_read_value<float>(input, ply_format));
                break;

            case PLY_V_FLOAT_VALUE:
                vvalues.push_back(ply_read_value<float>(input, ply_format));
                break;

            case PLY_V_IGNORE_FLOAT:
                ply_read_value<float>(input, ply_format);
                break;

            case PLY_V_IGNORE_DOUBLE:
                ply_read_value<double>(input, ply_format);
                break;

            case PLY_V_IGNORE_UINT32:
                ply_read_value<unsigned int>(input, ply_format);
                break;

            case PLY_V_IGNORE_UINT8:
                ply_read_value<unsigned char>(input, ply_format);
                break;

            default:
                throw std::runtime_error("Unhandled PLY vertex property");
            }
        }

        vertices.push_back(vertex);
        if (want_vnormals)
            vnormals.push_back(vnormal);
        if (want_colors)
            vcolors.push_back(color);
        if (want_tex_coords)
            tcoords.push_back(tex_coord);

        eof = input.eof();
    }

    /* Start reading the face data. */
    if (num_faces > 0)
        std::cout << " " << num_faces << " faces..." << std::flush;
    faces.reserve(num_faces * 3);
    for (std::size_t i = 0; !eof && i < num_faces; ++i)
    {
        for (std::size_t n = 0; n < f_format.size(); ++n)
        {
            PLYFaceProperty elem = f_format[n];
            switch (elem)
            {
                case PLY_F_VERTEX_INDICES:
                {
                    /* Read the amount of vertex indices for the face. */
                    unsigned char n_verts = ply_read_value<unsigned char>(input, ply_format);

                    if (n_verts == 3)
                    {
                        /* Process 3-vertex faces. */
                        faces.push_back(ply_read_value<unsigned int>(input, ply_format));
                        faces.push_back(ply_read_value<unsigned int>(input, ply_format));
                        faces.push_back(ply_read_value<unsigned int>(input, ply_format));
                    }
                    else if (n_verts == 4)
                    {
                        /* Process 4-vertex faces. */
                        unsigned int a = ply_read_value<unsigned int>(input, ply_format);
                        unsigned int b = ply_read_value<unsigned int>(input, ply_format);
                        unsigned int c = ply_read_value<unsigned int>(input, ply_format);
                        unsigned int d = ply_read_value<unsigned int>(input, ply_format);
#if 1
                        /* Interpret as tetmesh. */
                        faces.push_back(a);
                        faces.push_back(b);
                        faces.push_back(c);
                        faces.push_back(d);
#else
                        /* Triangulate 4-polygon (naive technique!). */
                        faces.push_back(a);
                        faces.push_back(b);
                        faces.push_back(c);

                        faces.push_back(c);
                        faces.push_back(d);
                        faces.push_back(a);
#endif
                    }
                    else if (!input.eof())
                    {
                        std::cout << "PLY Loader: Ignoring face with "
                            << static_cast<int>(n_verts)
                            << " vertices!" << std::endl;
                        for (int vid = 0; vid < n_verts; ++vid)
                            ply_read_value<unsigned int>(input, ply_format);
                    }
                    break;
                }

                case PLY_F_IGNORE_UINT32:
                    ply_read_value<int>(input, ply_format);
                    break;
                case PLY_F_IGNORE_UINT8:
                    ply_read_value<unsigned char>(input, ply_format);
                    break;
                case PLY_F_IGNORE_FLOAT:
                    ply_read_value<float>(input, ply_format);
                    break;

                default:
                    throw std::runtime_error("Unhandled PLY face property");
            }

            eof = input.eof();
        }
    }

    /* Start reading grid data. */
    if (!eof && num_grid != 0 && num_grid == grid_rows * grid_cols)
    {
        std::cout << " " << num_grid << " range grid values..." << std::flush;
        Image<unsigned int> img(grid_cols, grid_rows, 1);
        for (std::size_t i = 0; i < num_grid; ++i)
        {
            unsigned char indicator = ply_read_value<unsigned char>(input, ply_format);
            if (!indicator)
                img[i] = static_cast<unsigned int>(-1);
            else
                img[i] = ply_read_value<unsigned int>(input, ply_format);

            eof = input.eof();
        }
        mve::geom::rangegrid_triangulate(img, mesh);
    }

    /* Start reading triangle strips data. */
    if (!eof && num_tristrips)
    {
        std::cout << " triangle strips..." << std::flush;
        for (std::size_t i = 0; i < num_tristrips; ++i)
        {
            unsigned int num = ply_read_value<unsigned int>(input, ply_format);
            int last_id1 = -1, last_id2 = -1;
            bool inverted = false;
            for (unsigned int j = 0; j < num; ++j)
            {
                int idx = ply_read_value<int>(input, ply_format);
                if (idx < 0)
                {
                    last_id1 = -1;
                    last_id2 = -1;
                    inverted = false;
                    continue;
                }
                if (last_id1 != -1)
                {
                    faces.push_back(inverted ? last_id2 : last_id1);
                    faces.push_back(inverted ? last_id1 : last_id2);
                    faces.push_back(idx);
                    inverted = !inverted;
                }
                last_id1 = last_id2;
                last_id2 = idx;
            }
        }
    }

    /* Close the file stream. */
    input.close();
    std::cout << " done." << std::endl;
    if (eof)
        std::cout << "WARNING: Premature EOF detected!" << std::endl;

    return mesh;
}

/* ---------------------------------------------------------------- */

void
load_xf_file (std::string const& filename, float* ctw)
{
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    std::ifstream in(filename.c_str());
    if (!in.good())
        throw util::FileException(filename, std::strerror(errno));

    int values_read = 0;
    while (values_read < 16 && !in.eof())
    {
        in >> ctw[values_read];
        values_read += 1;
    }

    in.close();

    if (values_read < 16)
        throw util::Exception("Unexpected EOF");
}

/* ---------------------------------------------------------------- */

void
save_ply_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename,
    SavePLYOptions const& options)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    TriangleMesh::VertexList const& verts(mesh->get_vertices());
    TriangleMesh::ColorList const& vcolors(mesh->get_vertex_colors());
    TriangleMesh::NormalList const& vnormals(mesh->get_vertex_normals());
    TriangleMesh::ConfidenceList const& conf(mesh->get_vertex_confidences());
    TriangleMesh::ValueList const& vvalues(mesh->get_vertex_values());
    TriangleMesh::FaceList const& faces(mesh->get_faces());
    TriangleMesh::ColorList const& fcolors(mesh->get_face_colors());
    TriangleMesh::NormalList const& fnormals(mesh->get_face_normals());

    if (faces.size() % options.verts_per_simplex != 0)
        throw std::invalid_argument("Invalid amount of face indices");
    std::size_t face_amount = faces.size() / options.verts_per_simplex;

    bool write_vcolors = options.write_vertex_colors;
    write_vcolors = write_vcolors && verts.size() == vcolors.size();
    bool write_vnormals = options.write_vertex_normals;
    write_vnormals = write_vnormals && verts.size() == vnormals.size();
    bool write_vconfidences = options.write_vertex_confidences;
    write_vconfidences = write_vconfidences && verts.size() == conf.size();
    bool write_vvalues = options.write_vertex_values;
    write_vvalues = write_vvalues && verts.size() == vvalues.size();
    bool write_fcolors = options.write_face_colors && face_amount;
    write_fcolors = write_fcolors && fcolors.size() == face_amount;
    bool write_fnormals = options.write_face_normals && face_amount;
    write_fnormals = write_fnormals && fnormals.size() == face_amount;

    std::string format_str = (options.format_binary
        ? "binary_little_endian"
        : "ascii");

    /* Open output file. */
    std::ofstream out(filename.c_str(), std::ios::binary);
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    std::cout << "Writing PLY file (" << verts.size() << " verts"
        << (write_vcolors ? ", with colors" : "")
        << (write_vnormals ? ", with normals" : "")
        << (write_vconfidences ? ", with confidences" : "")
        << (write_vvalues ? ", with values" : "")
        << ", " << face_amount << " faces"
        << (write_fcolors ? ", with colors" : "")
        << (write_fnormals ? ", with normals" : "")
        << ")... " << std::flush;

    /* Generate PLY header. */
    out << "ply" << std::endl;
    out << "format " << format_str << " 1.0" << std::endl;
    out << "comment Export generated by libmve" << std::endl;
    out << "element vertex " << verts.size() << std::endl;
    out << "property float x" << std::endl;
    out << "property float y" << std::endl;
    out << "property float z" << std::endl;

    if (write_vnormals)
    {
        out << "property float nx" << std::endl;
        out << "property float ny" << std::endl;
        out << "property float nz" << std::endl;
    }

    if (write_vcolors)
    {
        out << "property uchar red" << std::endl;
        out << "property uchar green" << std::endl;
        out << "property uchar blue" << std::endl;
    }

    if (write_vconfidences)
    {
        out << "property float confidence" << std::endl;
    }

    if (write_vvalues)
    {
        out << "property float value" << std::endl;
    }

    if (face_amount)
    {
        out << "element face " << face_amount << std::endl;
        out << "property list uchar int vertex_indices" << std::endl;

        if (write_fnormals)
        {
            out << "property float nx" << std::endl;
            out << "property float ny" << std::endl;
            out << "property float nz" << std::endl;
        }

        if (write_fcolors)
        {
            out << "property uchar red" << std::endl;
            out << "property uchar green" << std::endl;
            out << "property uchar blue" << std::endl;
        }
    }

    out << "end_header" << std::endl;

    if (options.format_binary)
    {
        /* Output data in BINARY format. */
        for (std::size_t i = 0; i < verts.size(); ++i)
        {
            out.write((char const*)*verts[i], 3 * sizeof(float));
            if (write_vnormals)
                out.write((char const*)*vnormals[i], 3 * sizeof(float));
            if (write_vcolors)
            {
                unsigned char color[3];
                ply_color_convert(*vcolors[i], color);
                out.write((char const*)color, 3);
            }
            if (write_vconfidences)
                out.write((char const*)&conf[i], sizeof(float));
            if (write_vvalues)
                out.write((char const*)&vvalues[i], sizeof(float));
        }

        unsigned char verts_per_simplex_uchar = options.verts_per_simplex;
        for (std::size_t i = 0; i < face_amount; ++i)
        {
            out.write((char const*)&verts_per_simplex_uchar, 1);
            out.write((char const*)&faces[i * options.verts_per_simplex],
                options.verts_per_simplex * sizeof(unsigned int));
            if (write_fnormals)
                out.write((char const*)*fnormals[i], 3 * sizeof(float));
            if (write_fcolors)
            {
                unsigned char color[3];
                ply_color_convert(*fcolors[i], color);
                out.write((char const*)color, 3);
            }
        }
    }
    else
    {
        /* Output data in ASCII format. */
        out << std::fixed << std::setprecision(7);
        for (std::size_t i = 0; i < verts.size(); ++i)
        {
            out << verts[i][0] << " "
                << verts[i][1] << " "
                << verts[i][2];

            if (write_vnormals)
                for (int c = 0; c < 3; ++c)
                    out << " " << vnormals[i][c];
            if (write_vcolors)
                for (int c = 0; c < 3; ++c)
                {
                    float color = 255.0f * vcolors[i][c];
                    color = std::min(255.0f, std::max(0.0f, color));
                    out << " " << (int)(color + 0.5f);
                }
            if (write_vconfidences)
                out << " " << conf[i];
            if (write_vvalues)
                out << " " << vvalues[i];
            out << std::endl;
        }

        for (std::size_t i = 0; i < face_amount; ++i)
        {
            out << options.verts_per_simplex;
            for (unsigned int j = 0; j < options.verts_per_simplex; ++j)
                out << " " << faces[i * options.verts_per_simplex + j];
            if (write_fnormals)
                for (int j = 0; j < 3; ++j)
                    out << " " << fnormals[i][j];
            if (write_fcolors)
                for (int j = 0; j < 3; ++j)
                {
                    float color = 255.0f * fcolors[i][j];
                    color = std::min(255.0f, std::max(0.0f, color));
                    out << " " << (int)(color + 0.5f);
                }
            out << std::endl;
        }
    }

    out.close();
    std::cout << "done." << std::endl;
}

/* ---------------------------------------------------------------- */

void
save_ply_view (std::string const& filename, CameraInfo const& camera,
    FloatImage::ConstPtr depth_map, FloatImage::ConstPtr confidence_map,
    ByteImage::ConstPtr color_image)
{
    /* Some error and inconsistency checking. */
    if (depth_map == nullptr)
        throw std::invalid_argument("Null depth map given");
    //if (confidence_map == nullptr)
    //    throw std::invalid_argument("Null confidence map given");
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    int const width = depth_map->width();
    int const height = depth_map->height();
    math::Matrix3f invproj;
    camera.fill_inverse_calibration(*invproj, width, height);

    if (confidence_map != nullptr && (confidence_map->height() != height
        || confidence_map->width() != width))
        throw std::invalid_argument("Confidence map dimension does not match");

    if (color_image != nullptr && (color_image->width() != width
        || color_image->height() != height))
        throw std::invalid_argument("Color image dimension does not match");

    /* Open output file. */
    std::ofstream out(filename.c_str(), std::ios::binary);
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    std::cout << "Writing PLY file for image size "
        << width << "x" << height << "..." << std::endl;

    std::cout << "Counting... " << std::flush;

    /* Count valid depth values. */
    int num_verts = 0;
    int num_pixels = width * height;
    if (confidence_map != nullptr)
    {
        for (int i = 0; i < num_pixels; ++i)
            if (confidence_map->at(i, 0) > 0.0f)
                num_verts += 1;
    }
    else
    {
        for (int i = 0; i < num_pixels; ++i)
            if (depth_map->at(i, 0) > 0.0f)
                num_verts += 1;
    }

    std::cout << "(" << num_verts << "), " << std::flush;

    /* Write PLY header. */
    out << "ply" << std::endl;
    out << "format binary_little_endian 1.0" << std::endl;
    out << "comment Export generated by libmve" << std::endl;
    out << "obj_info num_cols " << width << std::endl;
    out << "obj_info num_rows " << height << std::endl;
    out << "element vertex " << num_verts << std::endl;
    out << "property float x" << std::endl;
    out << "property float y" << std::endl;
    out << "property float z" << std::endl;

    if (color_image != nullptr)
    {
        out << "property uchar red" << std::endl;
        out << "property uchar green" << std::endl;
        out << "property uchar blue" << std::endl;
    }

    if (confidence_map != nullptr)
    {
        out << "property float confidence" << std::endl;
    }

    out << "element range_grid " << (width * height) << std::endl;
    out << "property list uchar int vertex_indices" << std::endl;
    out << "end_header" << std::endl;

    /* Write out vertices, color and confidence. */
    std::cout << "writing vertices... " << std::flush;
    for (int i = 0; i < num_pixels; ++i)
    {
        int const x = i % width;
        int const y = i / width;
        int const yinv = height - y - 1;

        float confidence = 0.0f;
        if (confidence_map != nullptr)
        {
            confidence = confidence_map->at(x, yinv, 0);
            if (confidence <= 0.0f)
                continue;
        }

        float depth = depth_map->at(x, yinv, 0);
        if (depth <= 0.0f)
            continue;

        /* Build per-pixel viewing dir (in camera coords). */
        math::Vec3f pos = mve::geom::pixel_3dpos(x, yinv, depth, invproj);

        /* Convert vertex to world coords and write to file. */
        out.write((char const*)pos.begin(), 3 * sizeof(float));
        if (color_image != nullptr)
        {
            unsigned char const* c_off = &color_image->at(x, yinv, 0);
            out.write((char const*)c_off, 3);
        }

        if (confidence_map != nullptr)
            out.write((char const*)&confidence, sizeof(float));
    }

    std::cout << "writing range points... " << std::flush;

    /* Write out range points. */
    std::size_t vertex_id = 0;
    for (int i = 0; i < num_pixels; ++i)
    {
        int const x = i % width;
        int const y = i / width;
        int const yinv = height - y - 1;

        bool valid = true;
        if (confidence_map != nullptr && confidence_map->at(x, yinv, 0) <= 0.0f)
            valid = false;
        if (valid && depth_map->at(x, yinv, 0) <= 0.0f)
            valid = false;

        out.write((char const*)&valid, 1);
        if (valid)
        {
            out.write((char const*)&vertex_id, sizeof(int));
            vertex_id++;
        }
    }

    out.close();

    std::cout << "done." << std::endl;
}

/* ---------------------------------------------------------------- */

void
save_ply_view (View::Ptr view, std::string const& filename)
{
    save_ply_view(view, filename, "depthmap", "confidence", "undistorted");
}

/* ---------------------------------------------------------------- */

void
save_ply_view (View::Ptr view, std::string const& filename,
    std::string const& depthmap, std::string const& confidence,
    std::string const& color_image)
{
    if (view == nullptr)
        throw std::invalid_argument("Null view given");

    CameraInfo const& cam(view->get_camera());
    FloatImage::Ptr dm = view->get_float_image(depthmap);
    FloatImage::Ptr cm = view->get_float_image(confidence);
    ByteImage::Ptr ci = view->get_byte_image(color_image);
    save_ply_view(filename, cam, dm, cm, ci);
}

/* ---------------------------------------------------------------- */

void
save_xf_file (std::string const& filename, CameraInfo const& camera)
{
    math::Matrix4f ctw;
    camera.fill_cam_to_world(*ctw);
    save_xf_file(filename, *ctw);
}

/* ---------------------------------------------------------------- */

void
save_xf_file (std::string const& filename, float const* ctw)
{
    std::cout << "Writing XF file " << filename << "..." << std::endl;

    std::ofstream out(filename.c_str(), std::ios::binary);
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    for (std::size_t i = 0; i < 16; ++i)
        out << ctw[i] << (i % 4 == 3 ? "\n" : " ");

    out.close();
}

/* ---------------------------------------------------------------- */

FloatImage::Ptr
load_ply_depthmap (std::string const& filename)
{
    /* Precondition checks. */
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    /* Open file. */
    std::fstream input(filename.c_str());
    if (!input.good())
        throw util::FileException(filename, std::strerror(errno));

    /* Start parsing. */
    std::string buffer;
    input >> buffer; /* Read "ply" file signature. */
    if (buffer != "ply")
    {
        input.close();
        throw util::Exception("File format not recognized as PLY file");
    }

    /* Discard the rest of the line. */
    std::getline(input, buffer);

    std::size_t n_verts = 0;
    int n_grid = 0;
    int width = 0;
    int height = 0;
    while (input.good())
    {
        std::getline(input, buffer);
        util::string::clip_newlines(&buffer);
        util::string::clip_whitespaces(&buffer);

        util::Tokenizer t;
        t.split(buffer);

        if (t.empty())
            continue;

        if (t.size() == 3 && t[0] == "element" && t[1] == "vertex")
            n_verts = util::string::convert<std::size_t>(t[2]);
        if (t.size() == 3 && t[0] == "element" && t[1] == "range_grid")
            n_grid = util::string::convert<int>(t[2]);
        if (t.size() == 3 && t[0] == "obj_info" && t[1] == "num_cols")
            width = util::string::convert<int>(t[2]);
        if (t.size() == 3 && t[0] == "obj_info" && t[1] == "num_rows")
            height = util::string::convert<int>(t[2]);
        if (t.size() == 1 && t[0] == "end_header")
            break;
    }

    if (!n_verts || !n_grid || !width || !height || n_grid != width * height)
    {
        input.close();
        throw util::Exception("File headers not recognized as depthmap");
    }

    /* Read vertices. */
    TriangleMesh::VertexList verts;
    for (std::size_t i = 0; i < n_verts; ++i)
    {
        float x, y, z;
        input >> x >> y >> z;
        verts.push_back(math::Vec3f(x, y, z));
    }

    /* Discard newline. */
    std::getline(input, buffer);

    FloatImage::Ptr ret(FloatImage::create(width, height, 1));
    /* Read range grid. */
    for (int i = 0; i < n_grid; ++i)
    {
        // Flip y-axis
        int const idx = (height - (i / width) - 1) * width + (i % width);

        std::getline(input, buffer);
        util::string::clip_newlines(&buffer);
        util::string::clip_whitespaces(&buffer);
        util::Tokenizer t;
        t.split(buffer);

        if (input.eof())
        {
            std::cout << "Warning: Early EOF while parsing PLY" << std::endl;
            break;
        }

        if (t.empty() || t[0] != "1")
        {
            ret->at(idx) = 0.0f;
        }
        else
        {
            std::size_t vid = util::string::convert<std::size_t>(t[1]);
            if (vid >= verts.size())
            {
                std::cout << "Warning: Vertex index out of bounds" << std::endl;
                ret->at(idx) = 0.0f;
            }
            else
            {
                ret->at(idx) = verts[vid].norm();
            }
        }
    }

    input.close();
    return ret;
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
