#include "graphics/texture.h"
#include "script/scriptengine.h"
#include "script/scriptobject.h"
#include "script/scripthelper.h"
#include "graphics/window.h"

#include "freeimage.h"
#include <vector>

using namespace v8;

// Helps with setting up the script object.
class Texture::ScriptTexture : public ScriptObject<Texture> {

public:

  void Initialize()
  {
    ScriptObject::Initialize();
    AddAccessor("width", GetWidth);
    AddAccessor("height", GetHeight);
  }

  static void New(const FunctionCallbackInfo<Value>& args)
  {
    HandleScope scope(args.GetIsolate());
    ScriptHelper helper(args.GetIsolate());

    auto filename = ScriptEngine::GetCurrent().GetExecutionPath() + 
      helper.GetString(args[0]);

    try {
      auto scriptObject = new ScriptTexture(args.GetIsolate());
      auto object = scriptObject->Wrap(new Texture(filename));
      args.GetReturnValue().Set(object);
    }
    catch (std::exception& ex) {
      ScriptEngine::GetCurrent().ThrowTypeError(ex.what());
    }
  }

  static void GetWidth(
    Local<String> name, const PropertyCallbackInfo<Value>& args)
  {
    HandleScope scope(args.GetIsolate());
    auto self = Unwrap<Texture>(args.Holder());
    args.GetReturnValue().Set(self->GetWidth());
  }

  static void GetHeight(
    Local<String> name, const PropertyCallbackInfo<Value>& args)
  {
    HandleScope scope(args.GetIsolate());
    auto self = Unwrap<Texture>(args.Holder());
    args.GetReturnValue().Set(self->GetHeight());
  }

private:

  // Inherit constructors.
  using ScriptObject::ScriptObject;

};

Texture::Texture(std::string filename)
{
  Window::EnsureCurrentContext();

  // Load the image for the texture.
  auto fileType = FreeImage_GetFileType(filename.c_str());
  auto img = FreeImage_ConvertTo32Bits(
    FreeImage_Load(fileType, filename.c_str()));
  FreeImage_FlipVertical(img);

  if (!img) {
    throw std::runtime_error("Failed to load image '" + filename + "'");
  }

  width_ = FreeImage_GetWidth(img);
  height_ = FreeImage_GetHeight(img);

  // Create a new texture.
  glGenTextures(1, &glTexture_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, glTexture_);

  // Set texture filtering.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Set image data to the created texture.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_BGRA, 
    GL_UNSIGNED_BYTE, FreeImage_GetBits(img));

  FreeImage_Unload(img);
}

Texture::Texture(int width, int height, GLenum format)
{
  Window::EnsureCurrentContext();

  // Create a new texture.
  glGenTextures(1, &glTexture_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, glTexture_);

  // Empty texture data.
  std::vector<GLubyte> emptyData(width * height * 4, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, 
    GL_UNSIGNED_BYTE, &emptyData[0]);

  width_ = width;
  height_ = height;
}

Texture::~Texture()
{
  glDeleteTextures(1, &glTexture_);
}

void Texture::Bind(int unit)
{
  switch (unit) {
    case 1:
      glActiveTexture(GL_TEXTURE1);
      break;
    case 2:
      glActiveTexture(GL_TEXTURE2);
      break;
    case 3:
      glActiveTexture(GL_TEXTURE3);
      break;
    default:
      glActiveTexture(GL_TEXTURE0);
  }
  glBindTexture(GL_TEXTURE_2D, glTexture_);
}

void Texture::InstallScript(Isolate* isolate, Handle<ObjectTemplate> parent)
{
  ScriptTexture::Install<ScriptTexture>(isolate, "Texture", parent);
}