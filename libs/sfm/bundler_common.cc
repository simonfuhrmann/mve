#include <fstream>
#include <vector>
#include <sstream>
#include <cstring>
#include <cerrno>

#include "util/exception.h"
#include "sfm/bundler_common.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN


/* ----------------- Serialization of Descriptors ----------------- */

template <typename DESC, int LEN>
mve::ByteImage::Ptr
serialize_descriptors (std::vector<DESC> const& descriptors,
    int image_width, int image_height)
{
    /* Allocate storate for the descriptors. */
    std::size_t size_of_header = DESCR_SIGNATURE_LEN + 3 * sizeof(int32_t);
    std::size_t size_per_descriptor = 4 * sizeof(float) + LEN * sizeof(float);
    mve::ByteImage::Ptr data = mve::ByteImage::create
        (size_of_header + descriptors.size() * size_per_descriptor, 1, 1);

    /* Output stream to memory. */
    std::stringstream out;
    out.rdbuf()->pubsetbuf(data->get_byte_pointer(), data->get_byte_size());

    /* Write header. */
    int32_t num_descr = static_cast<int32_t>(descriptors.size());
    int32_t img_width = static_cast<int32_t>(image_width);
    int32_t img_height = static_cast<int32_t>(image_height);
    out.write(DESCR_SIGNATURE, DESCR_SIGNATURE_LEN);
    out.write(reinterpret_cast<char const*>(&num_descr), sizeof(int32_t));
    out.write(reinterpret_cast<char const*>(&img_width), sizeof(int32_t));
    out.write(reinterpret_cast<char const*>(&img_height), sizeof(int32_t));

    /* Write descriptors. */
    for (std::size_t i = 0; i < descriptors.size(); ++i)
    {
        DESC const& d = descriptors[i];
        out.write(reinterpret_cast<char const*>(&d.x), sizeof(float));
        out.write(reinterpret_cast<char const*>(&d.y), sizeof(float));
        out.write(reinterpret_cast<char const*>(&d.scale), sizeof(float));
        out.write(reinterpret_cast<char const*>(&d.orientation), sizeof(float));
        out.write(reinterpret_cast<char const*>(*d.data), sizeof(float) * LEN);
    }

    return data;
}

/* --------------- De-Serialization of Descriptors ---------------- */

template <typename DESC, int LEN>
void
deserialize_descriptors (mve::ByteImage::ConstPtr data,
    std::vector<DESC>* descriptors, int* width, int* height)
{
    if (descriptors == NULL)
        throw std::invalid_argument("NULL descriptors given");

    /* Input stream from memory. */
    std::istringstream in;
    in.rdbuf()->pubsetbuf(const_cast<char*>(data->get_byte_pointer()),
        data->get_byte_size());

    /* Read and check file signature. */
    char signature[DESCR_SIGNATURE_LEN + 1];
    in.read(signature, DESCR_SIGNATURE_LEN);
    signature[DESCR_SIGNATURE_LEN] = '\0';
    if (std::string(DESCR_SIGNATURE) != signature)
        throw std::invalid_argument("Error matching signature");

    /* Read header. */
    int32_t num_descr, img_width, img_height;
    in.read(reinterpret_cast<char*>(&num_descr), sizeof(int32_t));
    in.read(reinterpret_cast<char*>(&img_width), sizeof(int32_t));
    in.read(reinterpret_cast<char*>(&img_height), sizeof(int32_t));

    if (!in || in.eof() || num_descr < 0 || num_descr > 1000000)
        throw std::invalid_argument("Error reading header");

    /* Read descriptors. */
    descriptors->resize(num_descr);
    for (std::size_t i = 0; i < descriptors->size(); ++i)
    {
        DESC& d = descriptors->at(i);
        in.read(reinterpret_cast<char*>(&d.x), sizeof(float));
        in.read(reinterpret_cast<char*>(&d.y), sizeof(float));
        in.read(reinterpret_cast<char*>(&d.scale), sizeof(float));
        in.read(reinterpret_cast<char*>(&d.orientation), sizeof(float));
        in.read(reinterpret_cast<char*>(*d.data), sizeof(float) * LEN);
    }

    if (width != NULL)
        *width = img_width;
    if (height != NULL)
        *height = img_height;
}

/* -------------------- Descriptor conversions -------------------- */

mve::ByteImage::Ptr
descriptors_to_embedding (Sift::Descriptors const& descriptors,
    int width, int height)
{
    return serialize_descriptors<Sift::Descriptor, 128>
        (descriptors, width, height);
}

void
embedding_to_descriptors (mve::ByteImage::ConstPtr data,
    Sift::Descriptors* descriptors, int* width, int* height)
{
    deserialize_descriptors<Sift::Descriptor, 128>
        (data, descriptors, width, height);
}

mve::ByteImage::Ptr
descriptors_to_embedding (Surf::Descriptors const& descriptors,
    int width, int height)
{
    return serialize_descriptors<Surf::Descriptor, 64>
        (descriptors, width, height);
}

void
embedding_to_descriptors (mve::ByteImage::ConstPtr data,
    Surf::Descriptors* descriptors, int* width, int* height)
{
    deserialize_descriptors<Surf::Descriptor, 64>
        (data, descriptors, width, height);
}

/* -------------- Input/Output for Feature Matching --------------- */

void
save_pairwise_matching (PairwiseMatching const& matching,
    std::string const& filename)
{
    std::ofstream out(filename.c_str());
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    /* Write file signature and header. */
    out.write(MATCHING_SIGNATURE, MATCHING_SIGNATURE_LEN);
    int32_t num_pairs = static_cast<int32_t>(matching.size());
    out.write(reinterpret_cast<char const*>(&num_pairs), sizeof(int32_t));

    /* Write matching result. */
    for (std::size_t i = 0; i < matching.size(); ++i)
    {
        TwoViewMatching const& tvr = matching[i];
        int32_t id1 = static_cast<int32_t>(tvr.view_1_id);
        int32_t id2 = static_cast<int32_t>(tvr.view_2_id);
        out.write(reinterpret_cast<char const*>(&id1), sizeof(int32_t));
        out.write(reinterpret_cast<char const*>(&id2), sizeof(int32_t));
        int32_t num_matches = tvr.matches.size();
        out.write(reinterpret_cast<char const*>(&num_matches), sizeof(int32_t));
        for (std::size_t j = 0; j < tvr.matches.size(); ++j)
        {
            CorrespondenceIndex const& c = tvr.matches[j];
            int32_t i1 = static_cast<int32_t>(c.first);
            int32_t i2 = static_cast<int32_t>(c.second);
            out.write(reinterpret_cast<char const*>(&i1), sizeof(int32_t));
            out.write(reinterpret_cast<char const*>(&i2), sizeof(int32_t));
        }
    }

    out.close();
}

void
load_pairwise_matching (std::string const& filename,
    PairwiseMatching* matching)
{
    std::ifstream in(filename.c_str());
    if (!in.good())
        throw util::FileException(filename, std::strerror(errno));

    /* Read and check file signature. */
    char signature[MATCHING_SIGNATURE_LEN + 1];
    in.read(signature, MATCHING_SIGNATURE_LEN);
    signature[MATCHING_SIGNATURE_LEN] = '\0';
    if (std::string(MATCHING_SIGNATURE) != signature)
    {
        in.close();
        throw std::invalid_argument("Error matching signature");
    }

    matching->clear();

    /* Read matching result. */
    int32_t num_pairs;
    in.read(reinterpret_cast<char*>(&num_pairs), sizeof(int32_t));
    for (int32_t i = 0; i < num_pairs; ++i)
    {
        int32_t id1, id2;
        in.read(reinterpret_cast<char*>(&id1), sizeof(int32_t));
        in.read(reinterpret_cast<char*>(&id2), sizeof(int32_t));
        int32_t num_matches;
        in.read(reinterpret_cast<char*>(&num_matches), sizeof(int32_t));

        TwoViewMatching tvr;
        tvr.view_1_id = static_cast<int>(id1);
        tvr.view_2_id = static_cast<int>(id2);
        tvr.matches.reserve(num_matches);
        for (int32_t j = 0; j < num_matches; ++j)
        {
            CorrespondenceIndex c;
            int32_t i1, i2;
            in.read(reinterpret_cast<char*>(&i1), sizeof(int32_t));
            in.read(reinterpret_cast<char*>(&i2), sizeof(int32_t));
            c.first = static_cast<int>(i1);
            c.second = static_cast<int>(i2);
            tvr.matches.push_back(c);
        }
        matching->push_back(tvr);
    }

    if (in.eof())
    {
        in.close();
        throw util::Exception("Premature EOF");
    }

    in.close();
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END
