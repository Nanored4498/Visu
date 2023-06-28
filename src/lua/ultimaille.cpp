#include "ultimaille.h"

#include <lua/luabinder.h>
#include <lua/std.h>

#include <ultimaille/io/by_extension.h>

using namespace UM;

namespace Lua {
	void bindUltimaille() {
		addClass<SurfaceAttributes>("SurfaceAttributes");
		addClass<Triangles>("Triangles")
			.cons()
			.fun("nfacets", &Triangles::nfacets);
		bindPair<SurfaceAttributes, Triangles>("pairAttribTriangles");
		bindVector<std::pair<SurfaceAttributes, Triangles>>("vectorPairAttribTriangles");
		addFunction("read_by_extension_triangles", read_by_extension<Triangles>);
	}
}