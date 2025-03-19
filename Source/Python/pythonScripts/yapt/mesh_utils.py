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
    