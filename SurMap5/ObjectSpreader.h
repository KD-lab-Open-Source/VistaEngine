#include <map>
#include <stack>
#include <vector>
#include "Handle.h"

#include "..\Util\Range.h"

class ObjectSpreader {
public:
	struct Node : public ShareHandleBase {
		Node () : index (0)
		{}

		ShareHandle<Node> next;
		ShareHandle<Node> prev;
		int index;
	};

    struct Circle {
		Circle()
		: active (true)
		{}
        inline bool intersect (const Circle& rhs) const {
            return (rhs.position - position).norm2 () < (radius + rhs.radius) * (radius + rhs.radius);
        }
        Vect2f position;
        float radius;
		bool active;
    };


    ObjectSpreader ();
    void setRadius (const Rangef& _radius) { radius_ = _radius; }

    int getNextCircle ();

	void updateObjects ();
	void clear ();

	template<class Pred>
	void fill (Pred placementChecker)
	{
		clear ();
		newRadius ();

		Circle c;
		c.position.set (0.0f, 0.0f);
		c.radius = newRadius ();
		objects_.push_back (c);
		outline_ = new Node;
		outline_->index = 0;
		outline_->next = outline_;
		outline_->prev = outline_;

		if (!placementChecker (c))
			return;

		for(;;) {
			int count = outlineLength ();
			int offset = getNextCircle (); //round(float(rand()) * float(count) / (RAND_MAX + 1));

			int lastOne = addCircle (newRadius(), advanceNode (outline_, offset));
			getCircle(lastOne).active = placementChecker (getCircle (lastOne));

			Node* current = outline_;
			bool inactive = true;
			do {
				if (getCircle (current).active) {
					inactive = false;
					break;
				}
				current = current->next;
			} while (current != outline_);
			if (inactive || objects_.size() > 1500) {
				break;
			}
		}
		//eraseInactive ();
	}

    Circle& getCircle (int index);
    Circle& getCircle (Node* node);
    Node* getOutline () { return outline_; }
	int outlineLength ();

    typedef std::vector<Circle> ObjectsList;
	const ObjectsList& objects () { return objects_; }
    inline float newRadius () {
		float result = nextRadius_;
		nextRadius_ = (float(rand()) * radius_.length() / (RAND_MAX + 1.0f)) + radius_.minimum();
        return result;
    }
	inline float nextRadius () {
		return nextRadius_;
	}
	int addCircle (float radius, Node* node);
	int addCircle (float radius, int outlineIndex);
	void eraseInactive ();
private:
	float angle (Node* node1, Node* node2, Node* node3);
    Circle thirdCircle (const Circle& c1, const Circle& c2, float radius);
    Circle adjacentCircle (const Circle& circle, float radius);
    void repairOutline ();
	void eraseNode (Node* node);
	Node* advanceNode (Node*, int offset);
private:
    Rangef radius_;
    ObjectsList objects_;
	ShareHandle<Node> outline_;
	float nextRadius_;
    //ObjectsMap objects_;
};
