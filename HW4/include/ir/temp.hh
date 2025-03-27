#ifndef _TEMP
#define _TEMP

#include <map>
#include <string>
#include <cstdio>

using namespace std;

namespace tree {

class Temp {
  public:
    int num;
    Temp(int num) : num(num) {}
    int name() {
      return num;
    }
    string str() {
      return "t" + to_string(num);
    }
<<<<<<< HEAD
=======
    //temp equality is just based on the number, not the object!
    //That is: two test if two temp are the same, you need to check if their numbers are the same!
>>>>>>> ef84cc3dd4881fc9e0bf651ed964c46e6c13de56
    bool operator==(const Temp& t) const {
      return num == t.num;
    }
};

class Label {
  public:
    int num;
    Label(int num) : num(num) {}
    int name() {
      return num;
    }
    string str() {
      return "L" + to_string(num);
    }
<<<<<<< HEAD
=======
    //label equality is just based on the number, not the object!
    //That is: two test if two label are the same, you need to check if their numbers are the same!
>>>>>>> ef84cc3dd4881fc9e0bf651ed964c46e6c13de56
    bool operator==(const Label& l) const {
      return num == l.num;
    }
};

class Temp_map {
  public:
    map<int, bool> t_map; // temp map
    map<int, bool> l_map; // label map
    int next_temp; // next temp
    int next_label; // next label

    Temp_map() {
<<<<<<< HEAD
      next_temp = 100;
=======
      next_temp = 100; //start from 100
>>>>>>> ef84cc3dd4881fc9e0bf651ed964c46e6c13de56
      next_label = 100;
      t_map.clear();
      l_map.clear();
    }

    Temp* newtemp() {
<<<<<<< HEAD
      while (t_map[next_temp]) {
=======
      while (t_map[next_temp]) { //just to make sure the temp is unique (in terms of nnumber/name)
>>>>>>> ef84cc3dd4881fc9e0bf651ed964c46e6c13de56
        next_temp++;
      }
      t_map[next_temp] = true;
      return new Temp(next_temp++);
    }

    Label* newlabel() {
<<<<<<< HEAD
      while (l_map[next_label]) {
=======
      while (l_map[next_label]) { //just to make sure the label is unique (in terms of nnumber/name)
>>>>>>> ef84cc3dd4881fc9e0bf651ed964c46e6c13de56
        next_label++;
      }
      l_map[next_label] = true;
      return new Label(next_label++);
    }

    // get the temp with its name (number) if it exists (else return nullptr)
    Temp* named_temp(int num) {
      if (!t_map[num]) t_map[num] = true;
<<<<<<< HEAD
        return new Temp(num);
=======
        return new Temp(num); //even if the same numbered temp exists, we return a new object 
        //but note that the equality of temps are based on the number, not the object!
>>>>>>> ef84cc3dd4881fc9e0bf651ed964c46e6c13de56
    }

    Label* named_label(int num) {
      if (!l_map[num]) l_map[num] = true;
<<<<<<< HEAD
      return new Label(num);
=======
      return new Label(num);  //even if the same numbered label exists, we return a new object
        //but note that the equality of labels are based on the number, not the object!
>>>>>>> ef84cc3dd4881fc9e0bf651ed964c46e6c13de56
    }

    bool in_tempmap(int num) {
      return t_map[num];
    }

    bool in_labelmap(int num) {
      return l_map[num];
    }
};

} // namespace tree

#endif
