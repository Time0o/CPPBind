namespace test
{

int add(int, int);

int add(int a, int b)
{ return a + b; }

class Adder;

class Adder
{
public:
  int add(int a, int b);

private:
  int _add(int a, int b);
};

int Adder::add(int a, int b)
{ return _add(a, b); }

int Adder::_add(int a, int b)
{ return a + b; }

} // namespace test
