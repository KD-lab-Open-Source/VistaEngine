class kdCell {
public:

	kdCell(const Vect2f& position, const Vect2f& endPosition, int colorIndex);
	void quant(float dt);

	Vect2f& position() { return position_; }
	Vect2f& endPosition() { return endPosition_; }
	void setAnchor(const Vect2f& position, float dt);

	int colorIndex() { return colorIndex_; }
	void start() {work_ = true; }
	void stop() {offPhase_ = true; }

	bool isWork() { return work_; }
	void setVelocity(Vect2f& velocity) { velocity_ = velocity; }

	float colorPhase;

private:

	Vect2f position_;
	Vect2f velocity_;
	Vect2f endPosition_;
	int colorIndex_;
	bool work_;
	bool offPhase_;

};