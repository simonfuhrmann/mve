/*
 * Copyright (C) 2024, Andre Schulz
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "mve/mesh_io_glb.h"
#include "mve/mesh_tools.h"
#include "util/exception.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

void
save_glb_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    TriangleMesh::VertexList const& verts(mesh->get_vertices());
    std::size_t const verts_size_bytes = verts.size() * sizeof(verts[0]);

    TriangleMesh::ColorList const& vcolors(mesh->get_vertex_colors());
    std::size_t const vcolors_size_bytes = vcolors.size() * sizeof(vcolors[0]);

    TriangleMesh::NormalList const& vnormals(mesh->get_vertex_normals());
    std::size_t const vnormals_size_bytes = vnormals.size() * sizeof(vnormals[0]);

    TriangleMesh::TexCoordList const& vtexcoords(mesh->get_vertex_texcoords());
    std::size_t const vtexcoords_size_bytes = vtexcoords.size() * sizeof(vtexcoords[0]);

    TriangleMesh::FaceList const& faces(mesh->get_faces());
    std::size_t const index_buf_size_bytes = faces.size() * sizeof(faces[0]);

    if (faces.size() % 3 != 0)
        throw std::invalid_argument("Triangle indices not divisible by 3");

    std::size_t total_bin_size_bytes = verts_size_bytes + vcolors_size_bytes
        + vnormals_size_bytes + vtexcoords_size_bytes + index_buf_size_bytes;

    /* Ensure binary buffer's end is aligned to 4-byte boundary according to
     * glTF 2.0 spec section 4.4.3.1.
     * https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#chunks-overview
     */
    std::size_t const bin_buf_padding_bytes = (4 - total_bin_size_bytes % 4) % 4;
    total_bin_size_bytes += bin_buf_padding_bytes;
    if (total_bin_size_bytes > std::numeric_limits<std::uint32_t>::max())
        throw std::length_error("Binary buffer exceeds uint32 limit!");

    /* Create glTF JSON. */
    std::stringstream ss;
    ss << "{";

    /* Write asset.
     * https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-asset
     */
    ss << "\"asset\":{"
            "\"generator\":\"MVE (https://github.com/simonfuhrmann/mve)\","
            "\"version\":\"2.0\""
          "},";

    /* Write buffers.
     * https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-buffer
     */
    ss << "\"buffers\":["
            "{\"byteLength\":" << total_bin_size_bytes << "}"
          "],";

    /* Write buffer views.
     * https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-bufferview
     */
    std::uint32_t constexpr GLTF_ARRAY_BUFFER = 34962;
    std::uint32_t constexpr GLTF_ELEMENT_ARRAY_BUFFER = 34963;

    std::uint32_t buffer_view_id_counter = 0;
    std::size_t byte_offset = 0;
    ss << "\"bufferViews\":[";

    /* Position buffer view. */
    std::uint32_t verts_buffer_view_id = buffer_view_id_counter++;
    ss << "{"
            "\"buffer\":0,"
            "\"byteOffset\":" << byte_offset << ","
            "\"byteLength\":" << verts_size_bytes << ","
            "\"target\":" << GLTF_ARRAY_BUFFER;
    ss << "}";
    byte_offset += verts_size_bytes;

    /* Color buffer view. */
    std::uint32_t vcolors_buffer_view_id = 0;
    if (!vcolors.empty())
    {
        vcolors_buffer_view_id = buffer_view_id_counter++;
        ss << ",{"
                "\"buffer\":0,"
                "\"byteOffset\":" << byte_offset << ","
                "\"byteLength\":" << vcolors_size_bytes << ","
                "\"target\":" << GLTF_ARRAY_BUFFER;
        ss << "}";
        byte_offset += vcolors_size_bytes;
    }

    /* Normal buffer view. */
    std::uint32_t vnormals_buffer_view_id = 0;
    if (!vnormals.empty())
    {
        vnormals_buffer_view_id = buffer_view_id_counter++;
        ss << ",{"
                "\"buffer\":0,"
                "\"byteOffset\":" << byte_offset << ","
                "\"byteLength\":" << vnormals_size_bytes << ","
                "\"target\":" << GLTF_ARRAY_BUFFER;
        ss << "}";
        byte_offset += vnormals_size_bytes;
    }

    /* Texcoord buffer view. */
    std::uint32_t vtexcoords_buffer_view_id = 0;
    if (!vtexcoords.empty())
    {
        vtexcoords_buffer_view_id = buffer_view_id_counter++;
        ss << ",{"
                "\"buffer\":0,"
                "\"byteOffset\":" << byte_offset << ","
                "\"byteLength\":" << vtexcoords_size_bytes << ","
                "\"target\":" << GLTF_ARRAY_BUFFER;
        ss << "}";
        byte_offset += vtexcoords_size_bytes;
    }

    /* Index buffer view. */
    std::uint32_t index_buffer_view_id = buffer_view_id_counter++;
    ss << ",{"
            "\"buffer\":0,"
            "\"byteOffset\":" << byte_offset << ","
            "\"byteLength\":" << index_buf_size_bytes << ","
            "\"target\":" << GLTF_ELEMENT_ARRAY_BUFFER;
    ss << "}";

    /* End of buffer views array. */
    ss << "],";

    /* Write accessors.
     * https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-accessor
     */
    std::uint32_t constexpr GLTF_UNSIGNED_INT = 5125;
    std::uint32_t constexpr GLTF_FLOAT = 5126;

    std::uint32_t accessor_id_counter = 0;
    ss << "\"accessors\":[";

    /* Vertex position accessor. */
    math::Vec3f aabb_min, aabb_max;
    geom::mesh_find_aabb(mesh, aabb_min, aabb_max);
    std::uint32_t verts_accessor_id = accessor_id_counter++;
    ss << "{"
            "\"bufferView\":" << verts_buffer_view_id << ","
            "\"componentType\":" << GLTF_FLOAT << ","
            "\"count\":" << verts.size() << ",";
    ss << std::setprecision(std::numeric_limits<float>::max_digits10);
    ss <<   "\"min\":[" << aabb_min[0] << "," << aabb_min[1] << "," << aabb_min[2] << "],"
            "\"max\":[" << aabb_max[0] << "," << aabb_max[1] << "," << aabb_max[2] << "],"
            "\"type\":\"VEC3\""
          "}";

    /* Color accessor. */
    std::uint32_t vcolors_accessor_id = 0;
    if (!vcolors.empty())
    {
        vcolors_accessor_id = accessor_id_counter++;
        ss << ",{"
                "\"bufferView\":" << vcolors_buffer_view_id << ","
                "\"componentType\":" << GLTF_FLOAT << ","
                "\"count\":" << vcolors.size() << ","
                "\"type\":\"VEC4\""
              "}";
    }

    /* Normal accessor. */
    std::uint32_t vnormals_accessor_id = 0;
    if (!vnormals.empty())
    {
        vnormals_accessor_id = accessor_id_counter++;
        ss << ",{"
                "\"bufferView\":" << vnormals_buffer_view_id << ","
                "\"componentType\":" << GLTF_FLOAT << ","
                "\"count\":" << vnormals.size() << ","
                "\"type\":\"VEC3\""
              "}";
    }

    /* Texcoord accessor. */
    std::uint32_t vtexcoords_accessor_id = 0;
    if (!vtexcoords.empty())
    {
        vtexcoords_accessor_id = accessor_id_counter++;
        ss << ",{"
                "\"bufferView\":" << vtexcoords_buffer_view_id << ","
                "\"componentType\":" << GLTF_FLOAT << ","
                "\"count\":" << vtexcoords.size() << ","
                "\"type\":\"VEC2\""
              "}";
    }

    /* Index buffer accessor. */
    std::uint32_t index_accessor_id = accessor_id_counter++;
    ss << ",{"
            "\"bufferView\":" << index_buffer_view_id << ","
            "\"componentType\":" << GLTF_UNSIGNED_INT << ","
            "\"count\":" << faces.size() << ","
            "\"type\":\"SCALAR\""
          "}";

    /* End of accessors array. */
    ss << "],";

    /* Write mesh and mesh primitive.
     * https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-mesh
     * https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-mesh-primitive
     */
    std::uint32_t constexpr GLTF_TRIANGLES = 4;
    ss << "\"meshes\":[{"
            "\"primitives\":[{"
                "\"attributes\":{"
                    "\"POSITION\":" << verts_accessor_id;
    if (!vcolors.empty())
        ss <<       ",\"COLOR_0\":" << vcolors_accessor_id;
    if (!vnormals.empty())
        ss <<       ",\"NORMAL\":" << vnormals_accessor_id;
    if (!vtexcoords.empty())
        ss <<       ",\"TEXCOORD_0\":" << vtexcoords_accessor_id;
    ss <<       "}," /* End of attributes object. */
                "\"indices\":" << index_accessor_id << ","
                "\"mode\":" << GLTF_TRIANGLES;
    ss <<   "}]" /* End of primitives array. */
          "}],"; /* End of meshes array. */

    /* Nodes and scene(s).
     * https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-node
     * https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-scene
     */
    ss << "\"nodes\":[{\"mesh\":0}],"
          "\"scene\":0,"
          "\"scenes\":[{\"nodes\":[0]}]";

    /* End of glTF JSON. */
    ss << "}";

    /* Ensure glTF JSON's end is aligned to 4-byte boundary with spaces
     * according to glTF 2.0 spec sections 4.4.3.1 [1] and 4.4.3.2 [2].
     * [1] https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#chunks-overview
     * [2] https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#structured-json-content
     */
    std::size_t const json_chunk_padding_bytes = (4 - ss.tellp() % 4) % 4;
    if (json_chunk_padding_bytes > 0)
        ss.write("   ", json_chunk_padding_bytes);

    std::string const json_chunk = ss.str();
    if (json_chunk.size() > std::numeric_limits<std::uint32_t>::max())
        throw std::length_error("JSON chunk exceeds uint32 limit!");
    std::uint32_t const json_chunk_len = static_cast<std::uint32_t>(json_chunk.size());

    std::size_t const glb_length = 12 + 8 + json_chunk_len + 8 + total_bin_size_bytes;
    if (glb_length > std::numeric_limits<std::uint32_t>::max())
        throw std::length_error("GLB length exceeds uint32 limit!");

    /* Open output file. */
    std::ofstream out(filename, std::ios::binary);
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    /* Write GLB header. */
    std::uint32_t const gltf_magic = 0x46546C67; /* "glTF" */
    out.write(reinterpret_cast<char const*>(&gltf_magic), sizeof(gltf_magic));

    std::uint32_t const gltf_version = 2;
    out.write(reinterpret_cast<char const*>(&gltf_version), sizeof(gltf_version));

    std::uint32_t const glb_length_u32 = static_cast<std::uint32_t>(glb_length);
    out.write(reinterpret_cast<char const*>(&glb_length_u32), sizeof(glb_length_u32));

    /* Write JSON chunk. */
    out.write(reinterpret_cast<char const*>(&json_chunk_len), sizeof(json_chunk_len));

    std::uint32_t const json_chunk_type = 0x4E4F534A; /* "JSON" */
    out.write(reinterpret_cast<char const*>(&json_chunk_type), sizeof(json_chunk_type));

    out.write(json_chunk.data(), json_chunk_len);

    /* Write binary buffer chunk. */
    std::uint32_t const total_bin_size_bytes_u32
        = static_cast<std::uint32_t>(total_bin_size_bytes);
    out.write(reinterpret_cast<char const*>(&total_bin_size_bytes_u32),
        sizeof(total_bin_size_bytes_u32));

    std::uint32_t const bin_chunk_type = 0x004E4942; /* "BIN\0" */
    out.write(reinterpret_cast<char const*>(&bin_chunk_type), sizeof(bin_chunk_type));

    out.write((char const*)verts.data(), verts_size_bytes);
    if (!vcolors.empty())
        out.write((char const*)vcolors.data(), vcolors_size_bytes);
    if (!vnormals.empty())
        out.write((char const*)vnormals.data(), vnormals_size_bytes);
    if (!vtexcoords.empty())
        out.write((char const*)vtexcoords.data(), vtexcoords_size_bytes);
    out.write((char const*)faces.data(), index_buf_size_bytes);

    /* Ensure binary buffer's end is aligned to 4-byte boundary with zeros
     * according to glTF 2.0 spec section 4.4.3.3.
     * https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#binary-buffer
     */
    if (bin_buf_padding_bytes > 0)
        out.write("\0\0\0", bin_buf_padding_bytes);
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
