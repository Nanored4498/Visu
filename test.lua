x = Foo.new();
x:bar();
print(x:f(10));
print(gen():f(10));

print("==================");

m = Triangles.new();
read_by_extension_triangles("../Quantization/input/bunny.obj", m);
print(m:nfacets());