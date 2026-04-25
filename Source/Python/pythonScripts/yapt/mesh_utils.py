from py_yapt import Renderer, ResourceUsageBits, ResourceDimension, VertexBufferLayout, MeshAttribute, ResourceFormat, AttributeSemanticName, Mesh
from yapt.conversions import ConvUtility
from pathlib import Path

from pygltflib import GLTF2
import numpy as np

class MeshUtility:
    def __init__(self):
        pass

    def calculate_tangents(positions, indices, uvs, normals):
        num_vertices = len(positions)
        tangents = np.zeros((num_vertices, 4))
        bitangents = np.zeros((num_vertices, 3))

        for i in range(0, len(indices), 3):
            i0, i1, i2 = indices[i:i+3]
            print("a")
            v0, v1, v2 = positions[i0], positions[i1], positions[i2]
            print("c")
            uv0, uv1, uv2 = uvs[i0], uvs[i1], uvs[i2]
            print("b")
            E1, E2 = v1 - v0, v2 - v0
            dUV1, dUV2 = uv1 - uv0, uv2 - uv0

            det = 1.0 / (dUV1[0] * dUV2[1] - dUV2[0] * dUV1[1])
            
            tangent = det * (dUV2[1] * E1 - dUV1[1] * E2)
            bitangent = det * (-dUV2[0] * E1 + dUV1[0] * E2)

            tangents[i0][:3] += tangent
            tangents[i1][:3] += tangent
            tangents[i2][:3] += tangent

            bitangents[i0] += bitangent
            bitangents[i1] += bitangent
            bitangents[i2] += bitangent
        
        print("s")
        for i in range(num_vertices):
            
            N = normals[i]
            T = tangents[i][:3]
            T = T - np.dot(N, T) * N
            T = T / np.linalg.norm(T)
            
            B = np.cross(N, T)
            w = 1.0 if np.dot(B, bitangents[i]) > 0 else -1.0

            tangents[i][:3] = T
            tangents[i][3] = w
        
        return tangents

    def generate_spherical_uvs(positions):
        x, y, z = positions[:, 0], positions[:, 1], positions[:, 2]

        r = np.linalg.norm(positions, axis=1)  
 
        theta = np.arccos(z / r)
        phi = np.arctan2(y, x)

        u = (phi + np.pi) / (2 * np.pi)
        v = theta / np.pi

        return np.column_stack((u, v))

    def calculate_arbitrary_tangents(normals):
        count = len(normals)
        tangents = np.zeros((count, 4))
        for i in range(count):
            normal = normals[i]
            x, y, z = MeshUtility.get_orthogonal_vec(*normal)
            tangents[i] = [x, y, z, 1]
        return tangents

    def get_orthogonal_vec(x, y, z):
        if x < y and x < z:
            return 0, -z, y
        elif y < z:
            return -z, 0, x
        else:
            return -y, x, 0
    

class MeshLoader:
    def __init__(self, renderer):
        self._renderer = renderer

    def create_and_upload_from_gltf(self, gltf, mesh_name, gltf_mesh, verbose = False):
        print(f"loading {mesh_name}")

        results = self._create_and_upload_from_gltf_impl(gltf, mesh_name, gltf_mesh, verbose)
        if results == None:
            results = ([], ())
        return results
    
    def _get_component_count(self, accessor):
        if(accessor.type == 'SCALAR'):
            return 1;
        elif(accessor.type == 'VEC2'):
            return 2;
        elif(accessor.type == 'VEC3'):
            return 3;
        elif(accessor.type == 'VEC4'):
            return 4;
        elif(accessor.type == 'MAT2'):
            return 4;
        elif(accessor.type == 'MAT3'):
            return 9;
        elif(accessor.type == 'MAT4'):
            return 16;

    def _get_component_size_in_bytes(self, accessor):
        if(accessor.componentType == 5120):
            return 1;
        elif(accessor.componentType == 5121):
            return 1;
        elif(accessor.componentType == 5122):
            return 2;
        elif(accessor.componentType == 5123):
            return 2;
        elif(accessor.componentType == 5125):
            return 4;
        elif(accessor.componentType == 5126):
            return 4;

    def _get_component_numpy_dtype(self, accessor):
        if(accessor.componentType == 5120):
            return np.int8
        elif(accessor.componentType == 5121):
            return np.uint8
        elif(accessor.componentType == 5122):
            return np.int16
        elif(accessor.componentType == 5123):
            return np.uint16
        elif(accessor.componentType == 5125):
            return np.uint32;
        elif(accessor.componentType == 5126):
            return np.float32

    def _extract_data_from_accessor(self, gltf, accessor_index):
        accessor = gltf.accessors[accessor_index]

        buffer_view = gltf.bufferViews[accessor.bufferView]
        buff = gltf.buffers[buffer_view.buffer]

        data = gltf.get_data_from_buffer_uri(buff.uri)

        comp_size_bytes = self._get_component_size_in_bytes(accessor)
        component_count = self._get_component_count(accessor)
        entry_count = accessor.count
        dtype = self._get_component_numpy_dtype(accessor)

        element_size = comp_size_bytes * component_count
        byte_stride = buffer_view.byteStride if getattr(buffer_view, "byteStride", None) else element_size

        start = (buffer_view.byteOffset or 0) + (accessor.byteOffset or 0)

        view = np.ndarray(
            shape=(entry_count, component_count),
            dtype=dtype,
            buffer=data,
            offset=start,
            strides=(byte_stride, comp_size_bytes),
        )

        data_array = np.ascontiguousarray(view)
        if component_count == 1:
            data_array = data_array.reshape(-1)

        return (entry_count, component_count, comp_size_bytes, data_array)

    def _try_prim_array_merge(self, prim_data_arr, checkValidity = True):

        if prim_data_arr == None or len(prim_data_arr) == 0:
            return None

        if len(prim_data_arr) == 1:
            return prim_data_arr[0][3]

        if checkValidity:
            first = prim_data_arr[0]
            for data in prim_data_arr:

                if first[1] != data[1] or first[2] != data[2]:
                    print(f"ignoring primitive, unexpected data layout")
                    print(f"{first[1]}, {first[2]}")
                    print(f"{data[1]}, {data[2]}")
                    return None

        return np.concatenate([t[3] for t in prim_data_arr])
        

   
    def _validate_mesh(self, mesh_name, gltf_mesh):
        ##validate that mesh contains at least positions, normals and tangents
        first_attributes = {k for k, v in vars(gltf_mesh.primitives[0].attributes).items() if v is not None}
        for primitive in gltf_mesh.primitives:
            if primitive.indices is None:
                print(f"could not load mesh {mesh_name}, no indices present")
                return False
            if primitive.attributes.POSITION is None:
                print(f"could not load mesh {mesh_name}, no positions present")
                return False
            if primitive.attributes.NORMAL is None:
                print(f"could not load mesh {mesh_name}, no normals present")
                return False
            if primitive.attributes.TANGENT is None:
                print(f"could not load mesh {mesh_name}, no tangents present")
                return False
            comp_attributes = {k for k, v in vars(primitive.attributes).items() if v is not None}
            same_attr_layout = first_attributes == comp_attributes

            if not same_attr_layout:
                print(f"could not load mesh {mesh_name}, primitives don't share same input layout'")
                print(first_attributes)
                print(comp_attributes)
                return False

        return True

    def _collect_indexed_attribute_names(self, attributes, prefix):
        names = [
            k for k in vars(attributes)
            if k.startswith(prefix) and getattr(attributes, k) is not None
        ]
        return sorted(names, key=lambda s: int(s.split("_")[1]))

    def _build_interleaved_buffer(self, attr_specs, attr_data, vertex_count):
        offsets = []
        sizes = []
        offset = 0
        for fmt, sem, sem_idx, dtype, comp_count in attr_specs:
            attr_size = np.dtype(dtype).itemsize * comp_count
            offsets.append(offset)
            sizes.append(attr_size)
            offset += attr_size
        stride = offset

        buf = np.zeros((vertex_count, stride), dtype=np.uint8)
        mesh_attrs = []
        for i, (fmt, sem, sem_idx, dtype, comp_count) in enumerate(attr_specs):
            attr_off = offsets[i]
            attr_size = sizes[i]
            data = np.ascontiguousarray(attr_data[i], dtype=dtype)
            data_bytes = data.view(np.uint8).reshape(vertex_count, attr_size)
            buf[:, attr_off:attr_off + attr_size] = data_bytes
            mesh_attrs.append(MeshAttribute(fmt, sem, sem_idx, attr_off))

        return buf, VertexBufferLayout(mesh_attrs, stride)

    def _create_and_upload_from_gltf_impl(self, gltf, mesh_name, gltf_mesh, verbose = False):
        print(f"loading {mesh_name}")

        if not self._validate_mesh(mesh_name, gltf_mesh):
            return None

        first_attrs = gltf_mesh.primitives[0].attributes
        texcoord_names = self._collect_indexed_attribute_names(first_attrs, "TEXCOORD_")
        color_names = self._collect_indexed_attribute_names(first_attrs, "COLOR_")

        #go through each primitive and append the data (need to offset indices to get submesh ranges)
        #GLTF primitives are not really submeshes: add logic to group meshes into submeshes when possible and otherwise create as many meshes as needed. For now return "mesh list" and mapping from primitive but just return one mesh
        faces_array =  []
        vertices_array = []
        normals_array = []
        tangents_array = []
        texcoords_arrays = {n: [] for n in texcoord_names}
        colors_arrays = {n: [] for n in color_names}

        for primitive in gltf_mesh.primitives:
            if primitive.indices is not None:
                faces = self._extract_data_from_accessor(gltf, primitive.indices)
                faces_array.append(faces)
        
            if primitive.attributes.POSITION is not None:
                vertices = self._extract_data_from_accessor(gltf, primitive.attributes.POSITION)
                vertices_array.append(vertices)
        
            if primitive.attributes.NORMAL is not None:
                normals = self._extract_data_from_accessor(gltf, primitive.attributes.NORMAL)
                normals_array.append(normals)
            
            if primitive.attributes.TANGENT is not None:
                tangents = self._extract_data_from_accessor(gltf, primitive.attributes.TANGENT)
                tangents_array.append(tangents)

            for n in texcoord_names:
                accessor_idx = getattr(primitive.attributes, n)
                texcoords_arrays[n].append(self._extract_data_from_accessor(gltf, accessor_idx))

            for n in color_names:
                accessor_idx = getattr(primitive.attributes, n)
                colors_arrays[n].append(self._extract_data_from_accessor(gltf, accessor_idx))

        faces = self._try_prim_array_merge(faces_array)
        vertices = self._try_prim_array_merge(vertices_array)
        normals = self._try_prim_array_merge(normals_array)
        tangents = self._try_prim_array_merge(tangents_array)
        texcoords = {n: self._try_prim_array_merge(texcoords_arrays[n]) for n in texcoord_names}
        colors = {n: self._try_prim_array_merge(colors_arrays[n]) for n in color_names}

        texcoord_names = [n for n in texcoord_names if texcoords.get(n) is not None]
        color_names = [n for n in color_names if colors.get(n) is not None]

        if vertices is None:
            print(f"could not load mesh {mesh_name}, missing vertex positions")
            return None

        if normals is None:
            print(f"could not load mesh {mesh_name}, missing vertex normals")
            return None

        #if tangents is None and texcoords:
            #print(f"Warning: mesh {mesh_name} does not contain tangents. Recalculating them (this might be slow)")
            #tangents = MeshUtility.calculate_tangents(vertices, faces, texcoords[texcoord_names[0]], normals) #TODO: need to handle submeshes, uncomment when done

        if tangents is None:
            print(f"could not load mesh {mesh_name}, missing vertex tangents (and no UVs to recalculate them)")
            return None

        layout = []
        buffers = []

        use16BitIndices = len(vertices) < 0xFFFF
        vertex_count = len(vertices)
        usage = ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.VERTEX_BUFFER | ResourceUsageBits.ACCELERATION_STRUCTURE_BUILD_INPUT

        layout.append(VertexBufferLayout([MeshAttribute(ResourceFormat.RGB32_SFLOAT, AttributeSemanticName.POSITION, 0)], VertexBufferLayout.STRIDE_TIGHTLY_PACKED))
        buffers.append(self._create_and_upload_buffer(f"{mesh_name}_positions", usage, vertices, np.float32))

        required_specs = [
            (ResourceFormat.RGB32_SFLOAT, AttributeSemanticName.NORMAL, 0, np.float32, 3),
            (ResourceFormat.RGBA32_SFLOAT, AttributeSemanticName.TANGENT, 0, np.float32, 4),
        ]
        required_data = [normals, tangents]
        required_buf, required_layout = self._build_interleaved_buffer(required_specs, required_data, vertex_count)
        layout.append(required_layout)
        buffers.append(self._create_and_upload_buffer(f"{mesh_name}_attrs", usage, required_buf, None))

        if texcoord_names or color_names:
            extra_specs = []
            extra_data = []
            for n in texcoord_names:
                idx = int(n.split("_")[1])
                extra_specs.append((ResourceFormat.RG32_SFLOAT, AttributeSemanticName.TEXCOORD, idx, np.float32, 2))
                extra_data.append(texcoords[n])
            for n in color_names:
                idx = int(n.split("_")[1])
                extra_specs.append((ResourceFormat.RGBA8_UINT, AttributeSemanticName.COLOR, idx, np.uint8, 4))
                extra_data.append(colors[n])

            extra_buf, extra_layout = self._build_interleaved_buffer(extra_specs, extra_data, vertex_count)
            layout.append(extra_layout)
            buffers.append(self._create_and_upload_buffer(f"{mesh_name}_uv_color", usage, extra_buf, None))

        indices = self._create_and_upload_buffer(f"{mesh_name}_indices", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.INDEX_BUFFER | ResourceUsageBits.ACCELERATION_STRUCTURE_BUILD_INPUT, faces, np.uint16 if use16BitIndices else np.uint32)

        submesh_count = len(faces_array)

        mesh = self._renderer.createMesh(mesh_name, layout, len(vertices), submesh_count, use16BitIndices) 

        for i in range(len(buffers)):
            mesh.setVertexBuffer(i, buffers[i], 0)

        mesh.setIndexBuffer(indices, 0)

        vertex_offset = 0
        index_offset = 0
        submesh_index = 0
        for submesh in faces_array:
            index_count = submesh[0]

            boundsMin, boundsMax = Mesh.calculateBoundsFromVertexBuffer(vertices.ctypes.data, faces.ctypes.data, index_offset, index_count, vertex_offset, 3 * 4, use16BitIndices)

            mesh.setSubmesh(submesh_index, index_offset, index_count, vertex_offset, boundsMin, boundsMax)
            
            vertex_offset += vertices_array[submesh_index][0]
            index_offset += index_count
            submesh_index += 1


        return ([mesh], tuple((0, i) for i in range(0, submesh_count)))

    def _create_and_upload_buffer(self, name, usage, data, forceType):

        if forceType != None:
            data = np.array(data, dtype=forceType)

        size = data.size * data.itemsize
        ptr = data.ctypes.data

        buff = self._renderer.createBuffer(name, usage, size)
        buff.upload(0, size, ptr, 0)
        return buff