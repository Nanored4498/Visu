x = Foo.new();
x:bar();
print(x:f(10));
x.y = 2;
print(x:f(10));
print(gen():f(10));
print(x.y, gen().y);

print("==================");

m = Triangles.new();
read_by_extension_triangles("../Quantization/input/bunny.obj", m);
print(m:nfacets());

print("==================");

