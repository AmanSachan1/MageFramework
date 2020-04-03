# How to List for future todo items

## Batched Drawcalls
- We need to use texture arrays for this. (Textures need to be of the same size so you would need to add up and down scaling for textures cpu side).
- Add a new element to the Vertex struct which determines the index in the texture array that a vertex/fragment would access in this batched draw call. 
- Create as many batched calls as necessary, starting with the smallest objects. (There are probably more efficient groupings but no need to hyper optimize this if it will only save you one or two draw calls (which is what would happen if you have relatively few meshes to draw)).
- Combine Vertex and index arrays upto their limits, I believe this is 65k (closest power of 2). 