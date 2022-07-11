#include "StdAfxRD.h"
#include <math.h>
#include "MultiRegion.h"
#include "serialization.h"

MultiRegion::Line MultiRegion::tempLine;

MultiRegion::MultiRegion(IntegerType width, IntegerType height, unsigned char fillType)
{
	init(width,height,fillType);
}

void MultiRegion::init(IntegerType width, IntegerType height, unsigned char fillType)
{
	width_=width;
	height_=height;
	lines_.clear();
    lines_.resize(height_);
	clear(fillType);
}


void MultiRegion::clear(unsigned char fillType)
{
	Interval left,right;
	left.x0 = 0;
	left.type = fillType;
	right.x0=width_;
	right.type = 0;

    for(IntegerType i = 0; i < height_; ++i)
	{
		lines_[i].resize(2);
        lines_[i][0]=left;
		lines_[i][1]=right;
	}
}


void MultiRegion::resize(IntegerType width, IntegerType height)
{
    width_ = width;
    height_ = height;
    lines_.resize(height_);
}

void MultiRegion::validate(const Line& line) const
{
    xassert(!line.empty());
    Interval prev = line.front();
	xassert(prev.x0==0);
	for(int i=1;i<line.size();i++)
	{
		const Interval& cur=line[i];
		if(i != line.size()-1)
	       xassert(prev.type != cur.type);
		xassert(cur.x0 > prev.x0);
		prev=cur;
    }
    xassert(prev.x0==width_);
	xassert(prev.type==0);
}

void MultiRegion::validate() const
{
    for(IntegerType y = 0; y < lines_.size(); ++y){
        validate(lines_[y]);
    }
}

void MultiRegion::fixRegion(unsigned char empty_type)
{
    for(IntegerType y = 0; y < lines_.size(); ++y)
	{
		Line& line=lines_[y];
		if(line.empty())
		{
			line.resize(2);
			line[0].x0=0;
			line[0].type=empty_type;
			line[1].x0=width_;
			line[1].type=0;
			continue;
		}
		
		if(line.front().x0!=0)
		{
			Interval p;
			p.x0=0;
			p.type=empty_type;
			line.insert(line.begin(),p);
		}

		Interval prev = line.front();
		xassert(prev.x0==0);
		for(int i=1;i<line.size();i++)
		{
			const Interval& cur=line[i];
			if(cur.x0 <= prev.x0)
			{
				line.erase(line.begin()+i);
				i--;
				continue;
			}

			if(cur.type == prev.type && i+1<line.size())
			{
				line.erase(line.begin()+i);
				i--;
				continue;
			}

			prev=cur;
		}

		if(line.back().x0!=width_)
		{
			if(line.back().x0<width_)
			{
				Interval p;
				p.x0=width_;
				p.type=0;
				line.push_back(p);
			}else
			{
				line.back().x0=width_;
			}
		}

		validate(line);
    }

}

void MultiRegion::addLine(Line& destLine, const Line& srcLine, IntegerType offset)
{
	tempLine.clear();
	xassert(!destLine.empty());
	if(srcLine.empty())
		return;

    Line::const_iterator src = srcLine.begin();
    Line::const_iterator dest = destLine.begin();
    Line::const_iterator endSrc  = srcLine.end();
    Line::const_iterator endDest = destLine.end();

	unsigned char tmp_type=0;//empty
	unsigned char dest_type=dest->type;

	if(src->type==0)
		src++;

	if(src->x0+offset<0)
	{
		unsigned char src_type=src->type;
		while(src!=endSrc && src->x0+offset<0)
		{
			src_type=src->type;
			src++;
		}

		if(src!=endSrc)
		{
			xassert(dest->x0==0);
			dest_type=dest->type;
			dest++;

			if(src_type)
			{
				Interval tmp;
				tmp.type=src_type;
				tmp.x0=0;
				tempLine.push_back(tmp);
				tmp_type=src_type;
			}
		}
	}

	while(dest!=endDest && src!=endSrc)
	{
		if(src->type)
		{
			while(dest!=endDest && dest->x0<src->x0+offset)
			{
				tempLine.push_back(*dest);
				tmp_type=dest_type=dest->type;
				dest++;
			}

			if(tmp_type!=src->type)
			{
				Interval tmp;
				tmp.type=src->type;
				tmp.x0=src->x0+offset;
				tempLine.push_back(tmp);
				tmp_type=src->type;
			}
		}else
		{
			while(dest!=endDest && dest->x0<=src->x0+offset)
			{
				dest_type=dest->type;
				dest++;
			}

			if(tmp_type!=dest_type)
			{
				Interval tmp;
				tmp.type=dest_type;
				tmp.x0=src->x0+offset;
				tempLine.push_back(tmp);
				tmp_type=dest_type;
			}
		}
		src++;
	}

	while(dest!=destLine.end())
	{
		tempLine.push_back(*dest);
		dest++;
	}

	if(tempLine.back().x0!=width_)
	{
		while(tempLine.back().x0>width_)
		{
			tempLine.pop_back();
		}

		if(tempLine.back().x0<width_)
		{
			Interval right;
			right.x0=width_;
			right.type = 0;
			tempLine.push_back(right);
		}
	}
	tempLine.back().type=0;

	destLine = tempLine;
}

void MultiRegion::add(const MultiRegion& source)
{
	xassert(width_ == source.width_ && height_ == source.height_);

	for(IntegerType y = 0; y < height_; ++y){
		const Line& srcLine = source.lines_[y];
		Line& destLine = lines_[y];
		addLine(destLine, srcLine);
#ifdef _DEBUG
		validate(destLine);
#endif
	}
}

void MultiRegion::operate(const ShapeRegion& shape)
{
	IntegerType first_y = max(0, shape.offsetY_);
	IntegerType max_y = min(height(), shape.offsetY_ + shape.height());

	IntegerType y;
	for(y = first_y; y < max_y; ++y){
		const Line& srcLine = shape.lines_[y - shape.offsetY_];
		Line& destLine = lines_[y];
		//addLine(destLine, srcLine, shape.offsetX_);
		addLine(destLine, srcLine, shape.offsetX_);
#ifdef _DEBUG
		validate(destLine);
#endif
	}
	//validate();
}

unsigned char MultiRegion::filled(int x,int y)
{
	if(y<0 || y>=height_ || x<0 || x>=width_)
		return 0;
	Line& line=lines_[y];
	unsigned char cur_type=0;
	for(Line::iterator it=line.begin();it!=line.end();++it)
	{
		if(it->x0<=x)
			cur_type=it->type;
		else
			break;
	}

	return cur_type;
}

ShapeRegion::ShapeRegion()
: MultiRegion(0, 0, 0)
, offsetX_(0)
, offsetY_(0)
{
}

void ShapeRegion::move(int deltaX, int deltaY)
{
    offsetX_ += deltaX;
    offsetY_ += deltaY;
}

void ShapeRegion::circle(int radius, unsigned char type)
{
	xassert(type);
	lines_.reserve(radius * 2 + 1);
    resize(radius * 2 + 1, radius * 2 + 1);
    for(int y = 0; y < height_; ++y){
		Line& line = lines_[y];
        
		int r2m=sqr(radius) - sqr(y - radius);
		int x = round(sqrtf(r2m));

		line.resize(2);
		IntegerType w = radius > x ? (radius - x) : (x - radius);
		Line::iterator i = line.begin();
		i->x0 = w;
		i->type = type;
		i++;
		i->x0 = w + x + x + 1;
		i->type = 0;
		i++;
    }
	offsetX_ = -radius;
	offsetY_ = -radius;
    //validate();
}

void ShapeRegion::box(int width, int height, unsigned char type)
{
    resize(width, height);
    Line line;
	Interval interval = {width, type};
    line.push_back(interval);
    for(IntegerType i = 0; i < height; ++i){
        lines_[i] = line;
    }
	offsetX_ = -width / 2;
	offsetY_ = -height / 2;
    //validate();
}

void MultiRegion::save(XBuffer& buffer) const
{
	buffer < width_;
	buffer < height_;
	buffer < lines_.size();
	Lines::const_iterator it;
	FOR_EACH(lines_, it){
		const Line& line = *it;
		buffer < line.size();

		Line::const_iterator lit;
		FOR_EACH(line, lit){
			buffer < lit->x0;
			buffer < lit->type;
		}
	}
}

void MultiRegion::load(XBuffer& buffer)
{
	buffer > width_;
	buffer > height_;
	xassert(width_ > 0 && height_ > 0 && width_ < 65536 && height_ < 65536);
	
	std::size_t lines_count = 0;
	buffer > lines_count;
	xassert(lines_count == height_);
	lines_.clear();
	lines_.resize(lines_count);

	Lines::iterator it;
	FOR_EACH(lines_, it){
		Line& line = *it;
		std::size_t line_size;
		buffer > line_size;

		line.resize(line_size);

		Line::iterator lit;
		FOR_EACH(line, lit){
			buffer > lit->x0;
			buffer > lit->type;
		}
	}
#ifdef _DEBUG
	validate();
#endif
}

void MultiRegion::serialize(Archive& ar)
{
	ar.serialize(width_, "width", 0);
    ar.serialize(height_, "height", 0);
	if(ar.isOutput()){
		XBuffer buffer(1024, 1);
		save(buffer);
		ar.serialize(buffer, "data", 0);
	}
	else{
		XBuffer buffer(1024, 1);
		if(ar.serialize(buffer, "data", 0) && buffer.tell()){
			buffer.set(0);
			load(buffer);
		}
	}
}

void MultiRegion::Interval::serialize(Archive& ar)
{
	ar.serialize(x0, "x0", 0);
	ar.serialize(type, "type", 0);
}
