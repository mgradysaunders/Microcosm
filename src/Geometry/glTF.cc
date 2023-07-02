#include "Microcosm/Geometry/glTF"
#include "Microcosm/Quaternion"

namespace mi::geometry::glTF {

void Asset::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .required("version", version)
    .optionalImplicit("copyright", copyright)
    .optionalImplicit("generator", generator)
    .optionalImplicit("minVersion", minVersion);
}

void Accessor::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .required("type", type)
    .required("componentType", component)
    .required("count", count)
    .optionalByDefault("bufferView", bufferView, BadIndex)
    .optionalByDefault("byteOffset", byteOffset)
    .optionalByDefault("min", minValues)
    .optionalByDefault("max", maxValues)
    .optionalByDefault("normalized", normalized)
    .optionalImplicit("sparse", sparse);
  WithNameAndExtensions::jsonConversion(conversion);
}

void Accessor::Sparse::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .required("count", count)
    .required("indices", indices)
    .required("values", values);
  WithExtensions::jsonConversion(conversion);
}

void Accessor::Sparse::Indices::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .required("componentType", component)
    .required("bufferView", bufferView)
    .optionalByDefault("byteOffset", byteOffset);
  WithExtensions::jsonConversion(conversion);
}

void Accessor::Sparse::Values::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .required("bufferView", bufferView)
    .optionalByDefault("byteOffset", byteOffset);
  WithExtensions::jsonConversion(conversion);
}

void Buffer::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .optionalByDefault("uri", uri)
    .required("byteLength", byteLength);
  WithNameAndExtensions::jsonConversion(conversion);
}

void BufferView::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .required("buffer", buffer)
    .required("byteLength", byteLength)
    .optionalByDefault("byteOffset", byteOffset)
    .optionalByDefault("byteStride", byteStride)
    .optionalByDefault("target", target, Target::None);
  WithNameAndExtensions::jsonConversion(conversion);
}

void TextureInfo::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .required("index", index)
    .optionalByDefault("texCoord", texcoord);
  WithExtensions::jsonConversion(conversion);
}

void Texture::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .optionalByDefault("source", imageSource, BadIndex)
    .optionalByDefault("sampler", sampler, BadIndex);
  WithNameAndExtensions::jsonConversion(conversion);
}

void Sampler::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .optionalByDefault("minFilter", minFilter, Filter::None)
    .optionalByDefault("magFilter", magFilter, Filter::None)
    .optionalByDefault("wrapS", wrap0, Wrap::None)
    .optionalByDefault("wrapT", wrap1, Wrap::None);
  WithNameAndExtensions::jsonConversion(conversion);
}

void Image::jsonConversion(Json::Conversion &conversion) {
  if (conversion.reading() ? conversion.current().has("uri") : bufferView == BadIndex)
    conversion.required("uri", uriOrMimeType);
  else
    conversion //
      .required("mimeType", uriOrMimeType)
      .required("bufferView", bufferView);
  WithNameAndExtensions::jsonConversion(conversion);
}

void Material::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .optionalImplicit("pbrMetallicRoughness", pbr)
    .optionalImplicit("normalTexture", normalTexture)
    .optionalImplicit("occlusionTexture", occlusionTexture)
    .optionalImplicit("emissiveTexture", emissiveTexture)
    .optionalByDefault("emissiveFactor", emissive, {0, 0, 0})
    .optionalByDefault("alphaMode", alphaMode)
    .optionalByDefault("alphaCutoff", alphaCutoff, 0.5f)
    .optionalByDefault("doubleSided", twoSided);
  WithNameAndExtensions::jsonConversion(conversion);
}

void Material::PbrMetallicRoughness::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .optionalByDefault("baseColorFactor", baseColor, {1, 1, 1, 1})
    .optionalImplicit("baseColorTexture", baseColorTexture)
    .optionalByDefault("metallicFactor", metallic, 0.0f)
    .optionalByDefault("roughnessFactor", roughness, 1.0f)
    .optionalImplicit("metallicRoughnessTexture", metallicRoughnessTexture);
  WithExtensions::jsonConversion(conversion);
}

void Material::NormalTextureInfo::jsonConversion(Json::Conversion &conversion) { TextureInfo::jsonConversion(conversion.optionalByDefault("scale", scale, 1.0f)); }

void Material::OcclusionTextureInfo::jsonConversion(Json::Conversion &conversion) {
  TextureInfo::jsonConversion( //
    conversion.optionalByDefault("strength", strength, 1.0f));
}

void Mesh::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .required("primitives", primitives)
    .optionalImplicit("weights", weights);
  WithNameAndExtensions::jsonConversion(conversion);
}

void Mesh::Primitive::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .required("attributes", attributes)
    .optionalByDefault("mode", mode, Mode::Triangles)
    .optionalByDefault("material", material, BadIndex)
    .optionalByDefault("indices", indices, BadIndex)
    .optionalImplicit("targets", targets);
  WithExtensions::jsonConversion(conversion);
}

void Skin::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .optionalByDefault("inverseBindMatrices", inverseBindMatrices, BadIndex)
    .optionalByDefault("skeleton", skeleton, BadIndex)
    .required("joints", joints);
  WithNameAndExtensions::jsonConversion(conversion);
}

void Animation::jsonConversion(Json::Conversion &conversion) {
  conversion.required("channels", channels);
  conversion.required("samplers", samplers);
  WithNameAndExtensions::jsonConversion(conversion);
}

void Animation::Channel::jsonConversion(Json::Conversion &conversion) {
  conversion.required("sampler", sampler);
  conversion.required("target", target);
  WithExtensions::jsonConversion(conversion);
}

void Animation::Sampler::jsonConversion(Json::Conversion &conversion) {
  conversion.required("input", input);
  conversion.required("output", output);
  conversion.optionalImplicit("interpolation", interpolation);
  WithExtensions::jsonConversion(conversion);
}

void Node::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .optionalImplicit("children", children)
    .optionalByDefault("camera", camera, BadIndex)
    .optionalByDefault("skin", skin, BadIndex)
    .optionalByDefault("mesh", mesh, BadIndex)
    .optionalImplicit("weights", weights);
  auto &current = conversion.current();
  if (conversion.reading()) {
    if (current.has("matrix"))
      transform = Affine(current["matrix"]);
    else {
      auto trs = TrsTransform{};
      if (current.has("translation")) trs.translation = current.at("translation");
      if (current.has("rotation")) trs.rotation = current.at("rotation");
      if (current.has("scale")) trs.scale = current.at("scale");
      transform = trs;
    }
  } else {
    if (std::holds_alternative<Affine>(transform))
      current["matrix"] = std::get<Affine>(transform);
    else {
      auto &trs = std::get<TrsTransform>(transform);
      current["translation"] = trs.translation;
      current["rotation"] = trs.rotation;
      current["scale"] = trs.scale;
    }
  }
}

void File::jsonConversion(Json::Conversion &conversion) {
  conversion //
    .required("asset", asset)
    .optionalImplicit("accessors", accessors)
    .optionalImplicit("animations", animations)
    .optionalImplicit("buffers", buffers)
    .optionalImplicit("bufferViews", bufferViews)
    .optionalImplicit("cameras", cameras)
    .optionalImplicit("images", images)
    .optionalImplicit("materials", materials)
    .optionalImplicit("meshes", meshes)
    .optionalImplicit("nodes", nodes)
    .optionalImplicit("samplers", samplers)
    .optionalByDefault("scene", scene, BadIndex)
    .optionalImplicit("scenes", scenes)
    .optionalImplicit("skins", skins)
    .optionalImplicit("textures", textures)
    .optionalImplicit("extensionsUsed", extensionsUsed)
    .optionalImplicit("extensionsRequired", extensionsRequired);
  WithExtensions::jsonConversion(conversion);
}

} // namespace mi::geometry::glTF
