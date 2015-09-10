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
#include <cstring>
#include <cerrno>

#include "util/exception.h"
#include "util/tokenizer.h"
#include "util/string.h"

#include "scene_addins/camera_sequence.h"

void
CameraSequence::read_file (std::string const& fname)
{
    std::ifstream in(fname.c_str());
    if (!in.good())
        throw util::FileException(fname, std::strerror(errno));

    util::Tokenizer ftok;

    /* Read input file, store relevant tokens. */
    while (!in.eof())
    {
        std::string line;
        std::getline(in, line);

        util::Tokenizer tok;
        tok.split(line, ' ');
        for (std::size_t i = 0; i < tok.size(); ++i)
        {
            if (tok[i].empty())
                continue;
            if (tok[i][0] == '#')
                break;
            ftok.push_back(tok[i]);
        }
    }

    in.close();

#if 0
    std::cout << "Tokens: ";
    for (std::size_t i = 0; i < ftok.size(); ++i)
        std::cout << ftok[i] << " ";
    std::cout << std::endl;
#endif

    this->seq.clear();
    this->upvec = math::Vec3f(0.0f, 0.0f, 1.0f);

    /* Parse tokens. */
    CameraSpline active_sequence;
    active_sequence.name = "FIRST";
    for (std::size_t i = 0; i < ftok.size(); ++i)
    {
        //std::cout << "Parsing token: " << ftok[i] << std::endl;
        if (ftok[i] == "fps")
        {
            this->set_fps(util::string::convert<int>(ftok[i+1]));
            i += 1;
            continue;
        }

        if (ftok[i] == "pause")
        {
            if (active_sequence.name != "FIRST")
                this->seq.push_back(active_sequence);
            active_sequence = CameraSpline();
            active_sequence.name = "pause";
            active_sequence.length = util::string::convert<std::size_t>(ftok[i+1]);
            i += 1;
            continue;
        }

        if (ftok[i] == "sequence")
        {
            if (active_sequence.name != "FIRST")
                this->seq.push_back(active_sequence);
            active_sequence = CameraSpline();
            active_sequence.name = ftok[i+1];
            i += 1;
            continue;
        }

        if (ftok[i] == "length")
        {
            active_sequence.length = util::string::convert<std::size_t>(ftok[i+1]);
            i += 1;
            continue;
        }

        if (ftok[i] == "lookat" || ftok[i] == "camera" || ftok[i] == "upvec")
        {
            math::Vec3f vec;
            vec[0] = util::string::convert<float>(ftok[i+1]);
            vec[1] = util::string::convert<float>(ftok[i+2]);
            vec[2] = util::string::convert<float>(ftok[i+3]);
            if (ftok[i] == "lookat")
                active_sequence.lookat.push_back(vec);
            else if (ftok[i] == "camera")
                active_sequence.camera.push_back(vec);
            else if (ftok[i] == "upvec")
                this->upvec = vec;
            i += 3;
            continue;
        }

        if (ftok[i] == "camera-spline-begin" || ftok[i] == "lookat-spline-begin")
        {
            bool camspline = (ftok[i] == "camera-spline-begin");
            while (ftok[i+1] != "camera-spline-end" && ftok[i+1] != "lookat-spline-end")
            {
                math::Vec3f vec;
                vec[0] = util::string::convert<float>(ftok[i+1]);
                vec[1] = util::string::convert<float>(ftok[i+2]);
                vec[2] = util::string::convert<float>(ftok[i+3]);
                if (camspline)
                    active_sequence.camera.push_back(vec);
                else
                    active_sequence.lookat.push_back(vec);
                i += 3;
            }
            i += 1;
            continue;
        }

        if (ftok[i] == "camera-knots" || ftok[i] == "lookat-knots")
        {
            bool camknots = (ftok[i] == "camera-knots");
            int degree = std::min((std::size_t)3, (camknots ? active_sequence.camera.size() - 1 : active_sequence.lookat.size() - 1));
            int points = camknots ? active_sequence.camera.size() : active_sequence.lookat.size();
            int knots = points + degree + 1;
            for (int j = 0; j < knots; ++j)
                if (camknots)
                    active_sequence.cs.add_knot(util::string::convert<float>(ftok[i+j+1]));
                else
                    active_sequence.ls.add_knot(util::string::convert<float>(ftok[i+j+1]));
            i += knots;
            continue;
        }

        throw util::Exception("Error: Unexpected token: ", ftok[i]);
    }

    if (active_sequence.name != "FIRST")
        this->seq.push_back(active_sequence);

    /* Setup splines. */
    for (std::size_t i = 0; i < this->seq.size(); ++i)
    {
        CameraSpline& s(this->seq[i]);
        std::cout << "Processing spline " << i << ": " << s.name << std::endl;
        for (std::size_t j = 0; j < s.camera.size(); ++j)
            s.cs.add_point(s.camera[j]);
        for (std::size_t j = 0; j < s.lookat.size(); ++j)
            s.ls.add_point(s.lookat[j]);

        if (!s.camera.empty())
        {
            int degree = std::min((std::size_t)3, s.camera.size() - 1);
            int knots = s.cs.get_knots().size();
            int points = s.cs.get_points().size();
            s.cs.set_degree(degree);

            if (knots != 0 && knots != points + degree + 1)
                std::cout << "    Warning: Invalid amount of camera knots!"
                    " Regenerating uniformly spaced knots." << std::endl;
            if (knots != points + degree + 1)
                s.cs.uniform_knots(0.0f, 1.0f);
            else
                s.cs.scale_knots(0.0f, 1.0f);
        }
        if (!s.lookat.empty())
        {
            int degree = std::min((std::size_t)3, s.lookat.size() - 1);
            int knots = s.ls.get_knots().size();
            int points = s.ls.get_points().size();
            s.ls.set_degree(degree);

            if (knots != 0 && knots != points + degree + 1)
                std::cout << "    Warning: Invalid amount of lookat knots!"
                    " Regenerating uniformly spaced knots." << std::endl;
            if (knots != points + degree + 1)
                s.ls.uniform_knots(0.0f, 1.0f);
            else
                s.ls.scale_knots(0.0f, 1.0f);
        }
    }
    std::cout << "Done setting up." << std::endl;
}

/* ----------------------------------------------------------------------- */

void
CameraSequence::write_file (std::string const& fname)
{
    std::ofstream out(fname.c_str(), std::ios::binary);
    if (!out.good())
        throw util::FileException(fname, std::strerror(errno));

    out << "fps " << this->fps << std::endl;
    out << "upvec " << this->upvec << std::endl;

    for (std::size_t i = 0; i < this->seq.size(); ++i)
    {
        CameraSpline& spline = this->seq[i];

        out << std::endl;

        if (spline.camera.empty() && spline.lookat.empty()
            && spline.cs.empty() && spline.ls.empty())
        {
            out << "pause " << spline.length << std::endl;
            continue;
        }

        out << "sequence " << spline.name << std::endl;
        out << "length " << spline.length << std::endl;

        out << "camera-spline-begin" << std::endl;
        for (std::size_t j = 0; j < spline.cs.get_points().size(); ++j)
            out << spline.cs.get_points()[j] << std::endl;
        out << "camera-spline-end" << std::endl;

        out << "camera-knots";
        for (std::size_t j = 0; j < spline.cs.get_knots().size(); ++j)
            out << ' ' << spline.cs.get_knots()[j];
        out << std::endl;

        out << "lookat-spline-begin" << std::endl;
        for (std::size_t j = 0; j < spline.ls.get_points().size(); ++j)
            out << spline.ls.get_points()[j] << std::endl;
        out << "lookat-spline-end" << std::endl;

        out << "lookat-knots";
        for (std::size_t j = 0; j < spline.ls.get_knots().size(); ++j)
            out << ' ' << spline.ls.get_knots()[j];
        out << std::endl;
    }
}

/* ----------------------------------------------------------------------- */

void
CameraSequence::transform (math::Matrix4f const& transf)
{
    this->upvec = transf.mult(this->upvec, 0.0f);

    for (std::size_t i = 0; i < this->seq.size(); ++i)
    {
        CameraSpline& spline = this->seq[i];
        spline.cs.transform(transf);
        spline.ls.transform(transf);
    }
}

/* ----------------------------------------------------------------------- */

bool
CameraSequence::next_frame (void)
{
    int cur_time = 0;
    std::size_t cur_seq = 0;
    while (true)
    {
        if (this->time < cur_time + this->seq[cur_seq].length)
            break;
        cur_time += this->seq[cur_seq].length;
        cur_seq += 1;
        if (cur_seq >= this->seq.size())
            return false;
    }

    CameraSpline const& spline(this->seq[cur_seq]);
    float t = float(this->time - cur_time) / float(spline.length);
    if (!spline.camera.empty())
        this->campos = spline.cs.evaluate(t);
    if (!spline.lookat.empty())
        this->lookat = spline.ls.evaluate(t);
    //this->upvec = math::Vec3f(0.0f, 0.0f, 1.0f);

    std::cout << "Sequence \"" << this->seq[cur_seq].name << "\""
        << ", frame " << this->frame << ", time " << this->time
        << ", t=" << t << std::endl;

    this->frame += 1;
    this->time += 1000 / this->fps;

    return true;
}

/* ----------------------------------------------------------------------- */

void
CameraSequence::apply_camera (ogl::Camera& camera)
{
    camera.pos = this->campos;
    camera.viewing_dir = (this->lookat - this->campos).normalized();
    camera.up_vec = this->upvec;
    camera.update_view_mat();
    camera.update_inv_view_mat();
}
