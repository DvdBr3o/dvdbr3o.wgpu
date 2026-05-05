#pragma once

#include "dvdbr3o/Render/Shader.hpp"

#include <slang.h>
#include <slang-cpp-prelude.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>
#include <ankerl/unordered_dense.h>

#include <string_view>
#include <span>
#include <cstddef>

#define DVDBR3O_EXTERN_RESOURCE(resource)                                                          \
	extern "C" const char dvdbr3o_embed_##resource##_start[];                                      \
	extern "C" const char dvdbr3o_embed_##resource##_end[];

#define DVDBR3O_EXTERN_SHADER(shader)                                                              \
	DVDBR3O_EXTERN_RESOURCE(shader##_slang)                                                        \
	DVDBR3O_EXTERN_RESOURCE(shader##_wgsl)                                                         \
	DVDBR3O_EXTERN_RESOURCE(shader##_layout_json)

#define DVDBR3O_RESOURCE(resource)                                                                 \
	::dvdbr3o_embed_##resource##_start,                                                            \
		static_cast<::std::size_t>(                                                                \
			::dvdbr3o_embed_##resource##_end - ::dvdbr3o_embed_##resource##_start                  \
		)
#define DVDBR3O_RESOURCE_OF(type, resource)                                                        \
	reinterpret_cast<type*>(::dvdbr3o_embed_##resource##_start),                                   \
		static_cast<::std::size_t>(                                                                \
			::dvdbr3o_embed_##resource##_end - ::dvdbr3o_embed_##resource##_start                  \
		)
#define DVDBR3O_RESOURCE_SPAN(type, resource)                                                      \
	std::span<type> {                                                                              \
		(type*)(::dvdbr3o_embed_##resource##_start),                                               \
			static_cast<::std::size_t>(                                                            \
				::dvdbr3o_embed_##resource##_end - ::dvdbr3o_embed_##resource##_start              \
			)                                                                                      \
	}

#define DVDBR3O_SHADER(shader)                                                                     \
	::dvdbr3o::Render::ExternShader {                                                              \
		.slang = ::dvdbr3o::Render::text_resource_view(DVDBR3O_RESOURCE(shader##_slang)),          \
		.wgsl  = ::dvdbr3o::Render::text_resource_view(DVDBR3O_RESOURCE(shader##_wgsl)),           \
		.layout_json =                                                                             \
			::dvdbr3o::Render::text_resource_view(DVDBR3O_RESOURCE(shader##_layout_json)),         \
	};

DVDBR3O_EXTERN_SHADER(Ascii)
DVDBR3O_EXTERN_SHADER(AsciiCompute)
DVDBR3O_EXTERN_SHADER(AsciiComposite)
DVDBR3O_EXTERN_SHADER(Mtoon)

DVDBR3O_EXTERN_RESOURCE(VRM1_Constraint_Twist_Sample_gltf)
DVDBR3O_EXTERN_RESOURCE(lab_vrm)

namespace dvdbr3o::Render {
inline constexpr auto text_resource_view(const char* begin, std::size_t size) -> std::string_view {
	while (size > 0 && begin[size - 1] == '\0') --size;
	return { begin, size };
}

struct ExternShader {
	const Render::SlangSource slang;
	const Render::WgslSource  wgsl;
	const Render::LayoutJson  layout_json;
};
}  // namespace dvdbr3o::Render

namespace dvdbr3o::Shaders {
inline static const Render::ExternShader ascii = DVDBR3O_SHADER(Ascii);
inline static const Render::ExternShader ascii_compute = DVDBR3O_SHADER(AsciiCompute);
inline static const Render::ExternShader ascii_composite = DVDBR3O_SHADER(AsciiComposite);
inline static const Render::ExternShader mtoon = DVDBR3O_SHADER(Mtoon);

class SlangSourceFilesystem : public ISlangFileSystem {
public:
	SlangSourceFilesystem() :
		_fs {
			{ "Ascii.slang", ascii.slang },
			{ "AsciiCompute.slang", ascii_compute.slang },
			{ "AsciiComposite.slang", ascii_composite.slang },
			{ "Mtoon.slang", mtoon.slang },
	} {}

	SLANG_NO_THROW SlangResult SLANG_MCALL
		loadFile(char const* path, ISlangBlob** outBlob) override {
		auto it = _fs.find(path);
		// if (it != _fs.end()) {
		// 	*outBlob = slang::RawBlob::create(it->second.data(), it->second.size()).detach();
		// 	return SLANG_OK;
		// }
		return SLANG_E_NOT_FOUND;
	}

private:
	ankerl::unordered_dense::map<std::string, Render::SlangSource> _fs;
};
}  // namespace dvdbr3o::Shaders

namespace dvdbr3o::Models {
inline static const auto vrm1_constraint_twist_sample_gltf =
	DVDBR3O_RESOURCE_SPAN(std::byte, VRM1_Constraint_Twist_Sample_gltf);
inline static const auto lab_vrm = DVDBR3O_RESOURCE_SPAN(std::byte, lab_vrm);

}  // namespace dvdbr3o::Models
