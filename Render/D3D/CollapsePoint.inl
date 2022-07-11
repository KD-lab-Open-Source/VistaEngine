Vect2i u_to_pos[U_ALL+1]=
{
	Vect2i(-1,0),//U_LEFT=0,
	Vect2i(1,0),//U_RIGHT=1,
	Vect2i(0,-1),//U_TOP=2,
	Vect2i(0,+1),//U_BOTTOM=3,
	Vect2i(-1,-1),//U_LEFT_TOP=4,
	Vect2i(+1,-1),//U_RIGHT_TOP=5,
	Vect2i(-1,+1),//U_LEFT_BOTTOM=6,
	Vect2i(+1,+1),//U_RIGHT_BOTTOM=7,
	Vect2i(0,0),//U_ALL=8,
};
/*
U_E pos_to_u[3][3];
void InitPosToU()
{
	for(int i=0;i<=U_ALL;i++)
	{
		Vect2i& p=u_to_pos[i];
		pos_to_u[p.x+1][p.y+1]=(U_E)i;
	}
}

U_E GetPosToU(int x,int y)
{

//	xassert(x>=-2 && x<=2);
//	xassert(y>=-2 && y<=2);
	x=clamp(x,-1,+1);
	y=clamp(y,-1,+1);

	return pos_to_u[x+1][y+1];
}
*/

U_E* InitPosToU();
static U_E pos_to_u[5*5];
static U_E* pos_to_u_offset=InitPosToU();
static U_E move_table[U_ALL+1][4];

U_E GetPosToU(int x,int y)
{
//	xassert(x>=-2 && x<=2);
//	xassert(y>=-2 && y<=2);
//	return pos_to_u[x+2+(y+2)*5];
	return pos_to_u_offset[x+y*5];
}

U_E Move(U_E move_to, U_E what)
{
	Vect2i& m=u_to_pos[move_to];
	Vect2i& w=u_to_pos[what];
	int x=m.x+w.x,y=m.y+w.y;
	//return GetPosToU(x,y);
	return pos_to_u_offset[x+y*5];
}

U_E* InitPosToU()
{
	for(int i=0;i<=U_ALL;i++)
	{
		Vect2i& p=u_to_pos[i];
		pos_to_u[p.x+2+(p.y+2)*5]=(U_E)i;
	}

	for(int x=-2;x<=+2;x++)
	for(int y=-2;y<=+2;y++)
	{
		int xx=clamp(x,-1,+1);
		int yy=clamp(y,-1,+1);
		pos_to_u[x+2+(y+2)*5]=pos_to_u[xx+2+(yy+2)*5];
	}

	pos_to_u_offset=pos_to_u+2+2*5;

	for(int move_to=0;move_to<=U_ALL;move_to++)
	{
		for(int what=0;what<4;what++)
			move_table[move_to][what]=Move((U_E)move_to,(U_E)what);
	}

	return pos_to_u_offset;
}


/*
bool sBumpTile::CollapsePoint(Vect2i& pround,int dx,int dy)
{
	int step=LOD;
	int xstep=1<<step;
	int xstep2=1<<(step-1);
	char dd=TILEMAP_SIZE>>step;
	pround.x=(dx+xstep2)>>step;
	pround.y=(dy+xstep2)>>step;

	if(pround.x>0 && pround.x<dd && 
		pround.y>0 && pround.y<dd)//Точка внктри тайла и не соприкасается с внешними.
	{
		return true;
	}

	//Граничные точки.
	//Если точка внутри, смотрим с какими граничит по осям x и y, и округляем.
	if(dx>=0 && dx<=TILEMAP_SIZE && dy>=0 && dy<=TILEMAP_SIZE)
	{
		if(pround.x==0)
		{
			if(border_lod[U_LEFT]>LOD)
				pround.y=((dy+xstep)>>(step+1))<<1;
			return true;
		}
		if(pround.x==dd)
		{
			if(border_lod[U_RIGHT]>LOD)
				pround.y=((dy+xstep)>>(step+1))<<1;
			return true;
		}

		if(pround.y==0)
		{
			if(border_lod[U_TOP]>LOD)
				pround.x=((dx+xstep)>>(step+1))<<1;
			return true;
		}
		if(pround.y==dd)
		{
			if(border_lod[U_BOTTOM]>LOD)
				pround.x=((dx+xstep)>>(step+1))<<1;
			return true;
		}
		xassert(0);
	}

	//Если точка снаружи, смотрим в какой тайл она попала, 
	//проверяем, граничная ли она, если не граничная - false,
	//если граничная, приводим к границе.
	int side=-1;
	if(dy<0)
	{
		if(dx<0)
			side=U_LEFT_TOP;
		else
		if(dx>TILEMAP_SIZE)
			side=U_RIGHT_TOP;
		else
			side=U_TOP;
	}else
	if(dy>TILEMAP_SIZE)
	{
		if(dx<0)
			side=U_LEFT_BOTTOM;
		else
		if(dx>TILEMAP_SIZE)
			side=U_RIGHT_BOTTOM;
		else
			side=U_BOTTOM;
	}else
	{
		if(dx<0)
			side=U_LEFT;
		else
		if(dx>TILEMAP_SIZE)
			side=U_RIGHT;
		else
			xassert(0);
	}
	xassert(side>=0);

	{//Вырезаем внутренние из других тайлов.
		int addx=0,addy=0;
		if(dx<0)
			addx=1;
		else
		if(dx>TILEMAP_SIZE)
			addx=-1;

		if(dy<0)
			addy=1;
		else
		if(dy>TILEMAP_SIZE)
			addy=-1;

		int dxx=dx+addx*TILEMAP_SIZE,dyy=dy+addy*TILEMAP_SIZE;
		Vect2i pround;
		int curLOD=border_lod[side];
		int step=curLOD;
		int xstep2=1<<(step-1);
		char dd=TILEMAP_SIZE>>step;
		pround.x=(dxx+xstep2)>>step;
		pround.y=(dyy+xstep2)>>step;

		if(pround.x>0 && pround.x<dd && 
			pround.y>0 && pround.y<dd)
		{
			return false;
		}
	}

	if(side==U_LEFT || side==U_RIGHT)
	{
		if(border_lod[side]==LOD)
		{
			if(pround.y==0 || pround.y==dd)
			{
				int add_side;
				if(pround.y==0)
					add_side=side==U_LEFT?U_LEFT_TOP:U_RIGHT_TOP;
				else
					add_side=side==U_LEFT?U_LEFT_BOTTOM:U_RIGHT_BOTTOM;

				if(border_lod[add_side]>LOD)
				{
					int xstep=1<<step;
					pround.x=((dx+xstep)>>(step+1))<<1;
				}

				if(side==U_LEFT)
					return pround.x==0;
				else
					return pround.x==dd;
			}
		}else
		if(border_lod[side]>LOD)
		{
			int xstep=1<<step;
			pround.x=((dx+xstep)>>(step+1))<<1;
			pround.y=((dy+xstep)>>(step+1))<<1;
		}else
		{
			xassert(border_lod[side]<LOD);
			int xstep4=1<<(step-2);
			pround.x=(dx+xstep4)>>(step-1);
			if(!(pround.x>=0 && pround.x<=dd*2))
				return false;
			pround.x=pround.x>>1;
			pround.y=(dy+xstep2)>>step;
		}

		if(pround.x>=0 && pround.x<=dd && pround.y>=0 && pround.y<=dd)
			return true;
		else
			return false;
	}

	if(side==U_TOP || side==U_BOTTOM)
	{
		if(border_lod[side]==LOD)
		{
			if(pround.x==0 || pround.x==dd)
			{
				int add_side;
				if(pround.x==0)
					add_side=side==U_TOP?U_LEFT_TOP:U_LEFT_BOTTOM;
				else
					add_side=side==U_TOP?U_RIGHT_TOP:U_RIGHT_BOTTOM;

				if(border_lod[add_side]>LOD)
				{
					int xstep=1<<step;
					pround.y=((dy+xstep)>>(step+1))<<1;
				}

				if(side==U_TOP)
					return pround.y==0;
				else
					return pround.y==dd;
			}

		}else
		if(border_lod[side]>LOD)
		{
			int xstep=1<<step;
			pround.x=((dx+xstep)>>(step+1))<<1;
			pround.y=((dy+xstep)>>(step+1))<<1;
		}else
		{
			xassert(border_lod[side]<LOD);
			int xstep4=1<<(step-2);
			pround.x=(dx+xstep2)>>step;
			pround.y=(dy+xstep4)>>(step-1);
			if(!(pround.y>=0 && pround.y<=dd*2))
				return false;
			pround.y=pround.y>>1;
		}

		if(pround.x>=0 && pround.x<=dd && pround.y>=0 && pround.y<=dd)
			return true;
		else
			return false;
	}

	int side_x,side_y;
	if(side==U_LEFT_TOP)
	{
		side_x=U_LEFT;
		side_y=U_TOP;
	}else
	if(side==U_RIGHT_TOP)
	{
		side_x=U_RIGHT;
		side_y=U_TOP;
	}else
	if(side==U_LEFT_BOTTOM)
	{
		side_x=U_LEFT;
		side_y=U_BOTTOM;
	}else
	if(side==U_RIGHT_BOTTOM)
	{
		side_x=U_RIGHT;
		side_y=U_BOTTOM;
	}else
		xassert(0);

	int curLOD=border_lod[side];
	if(border_lod[side_x]<LOD && curLOD<LOD)
	{
		int xstep4=1<<(step-2);
		pround.x=(dx+xstep4)>>step;
	}else
	if(border_lod[side_x]>LOD || curLOD>LOD)
	{
		int xstep=1<<step;
		pround.x=((dx+xstep)>>(step+1))<<1;
	}

	if(border_lod[side_y]<LOD && curLOD<LOD)
	{
		int xstep4=1<<(step-2);
		pround.y=(dy+xstep4)>>step;
	}else
	if(border_lod[side_y]>LOD || curLOD>LOD)
	{
		int xstep=1<<step;
		pround.y=((dy+xstep)>>(step+1))<<1;
	}

	if(side==U_LEFT_TOP)
		return pround.x==0 && pround.y==0;

	if(side==U_RIGHT_TOP)
		return pround.x==dd && pround.y==0;

	if(side==U_LEFT_BOTTOM)
		return pround.x==0 && pround.y==dd;

	if(side==U_RIGHT_BOTTOM)
		return pround.x==dd && pround.y==dd;

	xassert(0);
	pround.x=pround.y=0;
	return false;
}
/*/
//Как переделать. Если точка снаружи, проверяем тайл.
//Генерим border_lod для нее ищем точку, а потом сдвигаем обратно.

enum E_COLLAPSE
{
	E_OUT=0,
	E_IN,
	E_BORDER
};

E_COLLAPSE CollapsePointIn(Vect2i& pround,int dx,int dy,int LOD,char* border_lod)
{
	int step=LOD;
	int xstep2=1<<(step-1);
	char dd=TILEMAP_SIZE>>step;
	pround.x=(dx+xstep2)>>step;
	pround.y=(dy+xstep2)>>step;

	if(pround.x>0 && pround.x<dd && 
		pround.y>0 && pround.y<dd)//Точка внктри тайла и не соприкасается с внешними.
	{
		return E_IN;
	}

	//Граничные точки.
	//Если точка внутри, смотрим с какими граничит по осям x и y, и округляем.
	if(dx>=0 && dx<=TILEMAP_SIZE && dy>=0 && dy<=TILEMAP_SIZE)
	{
		int xstep=1<<step;
		if(pround.x==0)
		{
			if(border_lod[U_LEFT]>LOD)
				pround.y=((dy+xstep)>>(step+1))<<1;
			return E_BORDER;
		}
		if(pround.x==dd)
		{
			if(border_lod[U_RIGHT]>LOD)
				pround.y=((dy+xstep)>>(step+1))<<1;
			return E_BORDER;
		}

		if(pround.y==0)
		{
			if(border_lod[U_TOP]>LOD)
				pround.x=((dx+xstep)>>(step+1))<<1;
			return E_BORDER;
		}
		if(pround.y==dd)
		{
			if(border_lod[U_BOTTOM]>LOD)
				pround.x=((dx+xstep)>>(step+1))<<1;
			return E_BORDER;
		}
		xassert(0);
	}
	
	return E_OUT;
}

bool sBumpTile::CollapsePoint(Vect2i& pround,int dx,int dy)
{
	if(CollapsePointIn(pround,dx,dy,lod_vertex,border_lod))
	{
		return true;
	}

	//Если точка снаружи, смотрим в какой тайл она попала, 
	//проверяем, граничная ли она, если не граничная - false,
	//если граничная, приводим к границе.
	int side=-1;
	if(dy<0)
	{
		if(dx<0)
			side=U_LEFT_TOP;
		else
		if(dx>TILEMAP_SIZE)
			side=U_RIGHT_TOP;
		else
			side=U_TOP;
	}else
	if(dy>TILEMAP_SIZE)
	{
		if(dx<0)
			side=U_LEFT_BOTTOM;
		else
		if(dx>TILEMAP_SIZE)
			side=U_RIGHT_BOTTOM;
		else
			side=U_BOTTOM;
	}else
	{
		if(dx<0)
			side=U_LEFT;
		else
		if(dx>TILEMAP_SIZE)
			side=U_RIGHT;
		else
			xassert(0);
	}
//	xassert(side>=0);

	char side_border_lod[4];//Для уменьшения количества расчетов.
	char side_lod=border_lod[side];

	for(int u=0;u<4;u++)
	{
		//U_E u_in=Move((U_E)side,(U_E)u);
		U_E u_in=move_table[side][u];
		int lod_from;
		if(u_in==U_ALL)
			lod_from=lod_vertex;
		else
			lod_from=border_lod[u_in];

		side_border_lod[u]=lod_from;
	}


	Vect2i& offset=u_to_pos[side];
	int side_dx=dx-offset.x*TILEMAP_SIZE;
	int side_dy=dy-offset.y*TILEMAP_SIZE;

	if(CollapsePointIn(pround,side_dx,side_dy,side_lod,side_border_lod)!=E_BORDER)
	{
		return false;
	}

	int side_step=side_lod;
	char side_dd=TILEMAP_SIZE>>side_step;

	switch(side)
	{
	case U_LEFT:
		if(!(pround.x==side_dd))
			return false;
		break;
	case U_RIGHT:
		if(!(pround.x==0))
			return false;
		break;
	case U_TOP:
		if(!(pround.y==side_dd))
			return false;
		break;
	case U_BOTTOM:
		if(!(pround.y==0))
			return false;
		break;
	case U_LEFT_TOP:
		if(!(pround.x==side_dd && pround.y==side_dd))
			return false;
		break;
	case U_RIGHT_TOP:
		if(!(pround.x==0 && pround.y==side_dd))
			return false;
		break;
	case U_LEFT_BOTTOM:
		if(!(pround.x==side_dd && pround.y==0))
			return false;
		break;
	case U_RIGHT_BOTTOM:
		if(!(pround.x==0 && pround.y==0))
			return false;
		break;
	}

	pround.x=pround.x<<side_step;
	pround.y=pround.y<<side_step;

	pround.x+=offset.x*TILEMAP_SIZE;
	pround.y+=offset.y*TILEMAP_SIZE;

	{
		//xassert((pround.x>>step<<step)==pround.x);
		//xassert((pround.y>>step<<step)==pround.y);
	}

	int step=lod_vertex;
	pround.x=pround.x>>step;
	pround.y=pround.y>>step;


	char dd=TILEMAP_SIZE>>step;

	if(pround.x>=0 && pround.x<=dd && 
		pround.y>=0 && pround.y<=dd)//Точка внктри тайла и не соприкасается с внешними.
	{
		return true;
	}

	return false;
}
/**/