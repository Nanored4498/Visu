x = Foo.new();
x:bar();
print(x:f(10));
x.y = 2;
print(x:f(10));
print(gen():f(10));
print(x.y, gen().y);
getA().y = 42;
print(getA().y);

print("==================");
p = pairAttribTriangles.new();
p.first = read_by_extension_triangles("../Quantization/input/bunny.obj", p.second);
print(p.second:nfacets());
-- meshes = vectorPairAttribTriangles.new();
-- meshes.push_back();
-- table.insert(meshes, Triangles.new());
-- table.insert(meshes, Triangles.new());
-- read_by_extension_triangles("../Quantization/input/bunny.obj", meshes[1]);
-- read_by_extension_triangles("../Quantization/input/armadillo.obj", meshes[2]);
-- print(meshes[1]:nfacets(), meshes[2]:nfacets());

print("==================");

