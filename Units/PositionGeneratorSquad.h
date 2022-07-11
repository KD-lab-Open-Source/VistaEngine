
class PositionGeneratorSquad {

	int direction; // 0-3;
	int step;
	int maxNum;
	int curNum;
	bool zeroPos;
	Vect2i curVector;
	float scale;

public:

	void init(float radius_) {
		direction = step = curNum = 0;
		maxNum = 1;
		zeroPos = true;
		curVector = Vect2i::ZERO;
		scale = 2*radius_;
	}

	Vect2f get() {
		if(zeroPos) {
			zeroPos = false;		
			return curVector;
		} else {
			switch(direction) {
				case 0: curVector.y++;
						break;
				case 1: curVector.x++;
						break;
				case 2: curVector.y--;
						break;
				case 3: curVector.x--;
						break;
			}
			curNum++;
			if(curNum == maxNum) { curNum=0; step++; direction++; }
			if(step == 2) { maxNum++; step=0; }
			if(direction == 4) direction = 0;

			return Vect2f(curVector.x*scale, curVector.y*scale);
		}
	}
};
