#ifndef __VERTEX_DECLARATION_MACRO_H_INCLUDED__
#define __VERTEX_DECLARATION_MACRO_H_INCLUDED__

#define BEGIN_VERTEX_DECLARATION(Type) \
LPDIRECT3DVERTEXDECLARATION9 Type::declaration = 0; \
struct VertexDeclaration_##Type { \
	VertexDeclaration_##Type() { \
        static D3DVERTEXELEMENT9 elements[] = {

#define END_VERTEX_DECLARATION(Type) \
            , D3DDECL_END() \
        }; \
		cD3DRender::RegisterVertexDeclaration(Type::declaration, elements); \
	} \
} vertexDeclaration_##Type;

#define BEGIN_VERTEX_DECLARATION_EX(Type,Declaration) \
LPDIRECT3DVERTEXDECLARATION9 Type::Declaration = 0; \
struct VertexDeclaration_##Type##Declaration { \
	VertexDeclaration_##Type##Declaration() { \
        static D3DVERTEXELEMENT9 elements[] = {

#define END_VERTEX_DECLARATION_EX(Type,Declaration) \
            , D3DDECL_END() \
        }; \
		cD3DRender::RegisterVertexDeclaration(Type::Declaration, elements); \
	} \
} vertexDeclaration_##Type##Declaration;

#endif
