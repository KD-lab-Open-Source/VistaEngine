#ifndef __MULTI_REGION_H_INCLUDED__
#define __MULTI_REGION_H_INCLUDED__

#include <vector>
#include "XUtil.h"
#include "MTSection.h"
#include "Render\inc\rd.h"

/*
  Предположение что данные в виде 
    struct Interval
	{
        IntegerType length;
        unsigned char type;
    };
  1. неудобны для оперирования.
  2. мешают повысить скорость до линейно возможной.
  3. Приведет к дополнительным проверкам когда придется резать по [0..width)

  Пускай будет 
    struct Interval
	{
        IntegerType x0;
        unsigned char type;
    };

  x0 - начало интервала.

*/

class ShapeRegion;
class Archive;

class RENDER_API MultiRegion{
public:
	typedef short IntegerType;

    struct Interval
	{
        IntegerType x0;
        unsigned char type;
		void serialize(Archive& ar);
    };

    typedef std::vector<Interval> Line;
    typedef std::vector<Line> Lines;

    MultiRegion(IntegerType width = 0, IntegerType height = 0, unsigned char fillType = 0);
	void init(IntegerType width = 0, IntegerType height = 0, unsigned char fillType = 0);

	Lines& lines() { return lines_; }
	const Lines& lines() const { return lines_; }

	void add(const MultiRegion& source);
	void operate(const ShapeRegion& shape);

    IntegerType width() const{ return width_; }
	IntegerType height() const{ return height_; }

	void save(XBuffer& buffer) const;
	void load(XBuffer& buffer);

	void validate(const Line& line) const;
	void validate() const;
	void fixRegion(unsigned char empty_type);

	void clear(unsigned char fillType = 0);

	unsigned char filled(int x,int y);
	void serialize(Archive& ar);

	void lock() { lock_.lock(); }
	void unlock() { lock_.unlock(); }

	void addLine(Line& destLine, const Line& srcLine, IntegerType offset = 0);
protected:
	///устанавливает количество строк (старые данные, в строках придется удалить самому)
	void resize(IntegerType width, IntegerType height);

	IntegerType width_;
    IntegerType height_;
    Lines lines_;

	static MTSection lock_;
};

class RENDER_API ShapeRegion : public MultiRegion{
	friend MultiRegion;
public:
	ShapeRegion();
    void circle(int radius, unsigned char type = 1);
    void box(int width, int height, unsigned char type = 1);

    void move(int deltaX, int deltaY);
private:
	IntegerType offsetX_;
	IntegerType offsetY_;
};

#endif
