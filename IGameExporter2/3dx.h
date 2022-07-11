#ifndef __3DX_H_INCLUDED__
#define __3DX_H_INCLUDED__

enum ITPL
{
	ITPL_CONSTANT=0,
	ITPL_LINEAR,
	ITPL_SPLINE,
	ITPL_UNKNOWN,
};

const int C3DX_GRAPH_OBJECT=10;
const int C3DX_LOGIC_OBJECT=11;
const int C3DX_MAX_WEIGHTS=12;
const int C3DX_NON_DELETE_NODE=13;
const int C3DX_LOGIC_NODE=14;

const int C3DX_ANIMATION_HEAD=90;
const int C3DX_ANIMATION_GROUPS=100;
const int C3DX_ANIMATION_GROUP=101;
const int C3DX_AG_HEAD=102;
const int C3DX_AG_LINK=103;
const int C3DX_AG_LINK_MATERIAL=104;

const int C3DX_ANIMATION_CHAIN=150;
const int C3DX_AC_ONE=151;

const int C3DX_ANIMATION_CHAIN_GROUP=170;
const int C3DX_ACG_ONE=171;
const int C3DX_ANIMATION_VISIBLE_SETS = 172;
const int C3DX_AVS_ONE = 173;
const int C3DX_AVS_ONE_NAME = 174;
const int C3DX_AVS_ONE_OBJECTS = 175;
const int C3DX_AVS_ONE_VISIBLE_GROUPS = 176;
const int C3DX_AVS_ONE_VISIBLE_GROUP = 177;
const int C3DX_AVS_ONE_LODS = 178;
const int C3DX_AVS_ONE_RAW = 179;
const int C3DX_AVS_ONE_HEAD = 180;

const int C3DX_NODES=200;
const int C3DX_NODE=201;

const int C3DX_NODE_CHAIN=198;
const int C3DX_NODE_CHAIN_HEAD=199;
const int C3DX_NODE_HEAD=202;
const int C3DX_NODE_POSITION=203;
const int C3DX_NODE_SCALE=204;
const int C3DX_NODE_ROTATION=205;
const int C3DX_NODE_VISIBILITY=210;
const int C3DX_NODE_INIT_MATRIX=220;

const int C3DX_NODE_POSITION_INDEX=225;
const int C3DX_NODE_SCALE_INDEX=226;
const int C3DX_NODE_ROTATION_INDEX=227;
const int C3DX_NODE_VISIBILITY_INDEX=228;

const int C3DX_STATIC_CHAINS_BLOCK=230;

const int C3DX_MESHES=300;

const int C3DX_MESH_HEAD=309;
const int C3DX_MESH=310;
const int C3DX_MESH_FACES=311;

const int C3DX_MESH_VERTEX=312;
const int C3DX_MESH_NORMAL=313;
const int C3DX_MESH_UV=314;
const int C3DX_MESH_NAME=315;
const int C3DX_MESH_UV2=316;

const int C3DX_MESH_SKIN=320;

const int C3DX_MATERIAL_GROUP=400;
const int C3DX_MATERIAL=401;
const int C3DX_MATERIAL_HEAD=402;

const int C3DX_MATERIAL_STATIC=410;
const int C3DX_MATERIAL_TEXTUREMAP=420;

const int C3DX_MATERIAL_ANIM_OPACITY=421;
const int C3DX_MATERIAL_ANIM_UV=422;
const int C3DX_MATERIAL_ANIM_UV_DISPLACEMENT=423;

const int C3DX_MATERIAL_ANIM_OPACITY_INDEX=431;
const int C3DX_MATERIAL_ANIM_UV_INDEX=432;
const int C3DX_MATERIAL_ANIM_UV_DISPLACEMENT_INDEX=433;

const int C3DX_BASEMENT=500;
const int C3DX_BASEMENT_VERTEX=501;
const int C3DX_BASEMENT_FACES=502;

const int C3DX_LOGIC_BOUND=600;
const int C3DX_LOGIC_BOUND_LOCAL=601;
const int C3DX_LOGIC_BOUND_NODE_LIST=602;

const int C3DX_LIGHTS=700;
const int C3DX_LIGHT=701;
const int C3DX_LIGHT_HEAD=702;
const int C3DX_LIGHT_TEXTURE=703;

const int C3DX_LIGHT_ANIM_COLOR=703;
const int C3DX_LIGHT_ANIM_COLOR_INDEX=713;

const int C3DX_LOGOS=800;
const int C3DX_LOGO_COORD_OLD=801;
const int C3DX_LOGO_COORD=802;

const int C3DX_EFFECTS = 900;
const int C3DX_EFFECT_HEAD = 901;
const int C3DX_EFFECT_KEY = 902;

const int C3DX_SKIN_GROUPS = 1000;
const int C3DX_SKIN_GROUP = 1001;

const int C3DX_OTHER_INFO = 1100;
const int C3DX_BOUND_SPHERE = 1101;

const int C3DX_BUFFERS = 1200;
const int C3DX_BUFFERS_HEAD = 1201;
const int C3DX_BUFFER_VERTEX = 1203;
const int C3DX_BUFFER_INDEX = 1204;

const int C3DX_DUPLICATE_INFO_TO_LOD=1300;
const int C3DX_DUPLICATE_NODES=1301;
const int C3DX_DUPLICATE_MATERIALS=1302;
const int C3DX_ADDITIONAL_LOD1=1310;
const int C3DX_ADDITIONAL_LOD2=1311;

const int C3DX_CAMERA=1400;
const int C3DX_CAMERA_HEAD=1401;
#endif
