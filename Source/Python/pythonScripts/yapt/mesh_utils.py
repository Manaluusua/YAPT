import numpy as np

class MeshUtility:
    def __init__(self):
        pass

    def calculate_tangents(positions, indices, uvs, normals):
        num_vertices = len(positions)
        tangents = np.zeros((num_vertices, 3))
        bitangents = np.zeros((num_vertices, 3))
    
        for i in range(0, len(indices), 3):
            i0, i1, i2 = indices[i], indices[i+1], indices[i+2]
            
            v0, v1, v2 = positions[i0], positions[i1], positions[i2]
            uv0, uv1, uv2 = uvs[i0], uvs[i1], uvs[i2]
            
            E1, E2 = v1 - v0, v2 - v0
            dUV1, dUV2 = uv1 - uv0, uv2 - uv0

            det = 1.0 / (dUV1[0] * dUV2[1] - dUV2[0] * dUV1[1])

            tangent = det * (dUV2[1] * E1 - dUV1[1] * E2)
            bitangent = det * (-dUV2[0] * E1 + dUV1[0] * E2)

            tangents[i0] += tangent
            tangents[i1] += tangent
            tangents[i2] += tangent

            bitangents[i0] += bitangent
            bitangents[i1] += bitangent
            bitangents[i2] += bitangent
        
        for i in range(num_vertices):
            N = normals[i]
            T = tangents[i]
            
            T = T - np.dot(N, T) * N
            T = T / np.linalg.norm(T)
            
            B = np.cross(N, T)
            w = 1.0 if np.dot(B, bitangents[i]) > 0 else -1.0
            
            tangents[i] = np.append(T, w) 
        
        return tangents

        
