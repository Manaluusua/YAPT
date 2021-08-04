*destroy pending resources slow both on vk and dx12 (mutexed). Also Vk needs a better way to store the handle
*Both Vk and Dx12, when upload heap runs out of space, try to allocate temporary (also use for very large uploads?)
*Unify upload heap stuff somehow? 
*raytrace acc struct: pool scratch memory? currently allocated with implicit heap on dx12. 
*handle multiple rendergraphs in flight 
*Implement MeshLayoutID (so that different ID actually means different layout)
*Currently the preupload and upload are serialized, could allow both to execute conurrently and just sync with render start? 
*Add a way to have guarantee on scheduling nodes, thus allowing actual subpasses to be used (can't break renderpass across commandbuffers)