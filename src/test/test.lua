x = Foo.new();
x:bar();
print(x:f(10));
print(gen():f(10));
print(x.y, gen().y);

print("==================");

m = Triangles.new();
read_by_extension_triangles("../Quantization/input/bunny.obj", m);
print(m:nfacets());

print("==================");

-- y = gen2();
-- y:bar();
-- print(y:f(10), y.y, type(y.y));
-- y.y = 50;
-- print(y:f(10), y.y);
