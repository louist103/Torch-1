#include "TextureAtlasFactory.h"

#include <fcntl.h>
#include <unistd.h>
#include <numeric>

#include "Companion.h"
#include "utils/TextureUtils.h"
#include "n64graphics.h"
#include "stb_image_write.h"

ExportResult TextureAtlasHeaderExporter::Export(std::ostream &write, std::shared_ptr<IParsedData> data, std::string &entryName, YAML::Node &node, std::string *replacement) {
    return std::nullopt;
}

ExportResult TextureAtlasCodeExporter::Export(std::ostream &write, std::shared_ptr<IParsedData> data, std::string &entryName, YAML::Node &node, std::string *replacement) {
    return std::nullopt;
}

ExportResult TextureAtlasBinaryExporter::Export(std::ostream &write, std::shared_ptr<IParsedData> data, std::string &entryName, YAML::Node &node, std::string *replacement) {
    return std::nullopt;
}

ExportResult TextureAtlasModdingExporter::Export(std::ostream &write, std::shared_ptr<IParsedData> data, std::string &entryName, YAML::Node &node, std::string *replacement) {
    return std::nullopt;
}


std::optional<std::shared_ptr<IParsedData>> TextureAtlasFactory::parse(std::vector<uint8_t>& buffer, YAML::Node& data) {
    return std::nullopt;
}

typedef struct Vec2i {
    int x;
    int y;
}Vec2i;

// We need to add padding due to how the N64 blends textures
constexpr int PADDING_SIZE = 4;

static bool AttemptToPackImages(int atlasWidth, int atlasHeight,
    const std::vector<std::shared_ptr<AtlasedTextures>>& textures,
    std::unordered_map<std::shared_ptr<AtlasedTextures>, Vec2i>& imagePlacement) {
    Vec2i current;
    current.x = PADDING_SIZE;
    current.y = PADDING_SIZE;
    int currentHeight = 0;
    for (const auto& tex : textures) {
        uint32_t width = tex->texData->mWidth + PADDING_SIZE;
        uint32_t height = tex->texData->mHeight + PADDING_SIZE;

        if (current.x + width >= atlasWidth) {
            current.y += currentHeight;
            if (current.y == 1044) {
                int bp = 0;
            }
            currentHeight = 0;
            current.x = PADDING_SIZE;
            if (current.x + width >= atlasWidth) {
                goto placeFail;
            }

            if (current.y + height >= atlasHeight) {
                placeFail:
                imagePlacement.clear();
                return false;
            }

        }
        if (current.y + height >= atlasHeight) {
            goto placeFail;
        }
        imagePlacement[tex] = current;
        current.x += width;
        if (currentHeight < height)
            currentHeight = height;
    }
    // TODO look into setting the atlas X and Y directly once things are a little more stable
    for (const auto& tex : textures) {
        tex->texData->mAtlasX = imagePlacement[tex].x;
        tex->texData->mAtlasY = imagePlacement[tex].y;
    }

    return true;
}

static void BuildAtlasBinary(const std::unordered_map<std::shared_ptr<AtlasedTextures>, Vec2i>& imagePlacements, uint16_t atlasWidth, uint16_t atlasHeight) {
    size_t atlasSizeBytes = atlasWidth * atlasHeight * 4;
    char* textureData = (char*)calloc(1, atlasSizeBytes); // use RGBA32
    for (const auto& t : imagePlacements) {
        auto texData = t.first->texData.get();
        auto palette = t.first->palette.get();

        auto width = t.first->texData->mWidth;
        auto height = t.first->texData->mHeight;
        auto ulX = t.second.x;
        auto ulY = t.second.y;

        switch (t.first->texData->mFormat.type) {
            case TextureType::RGBA32bpp:
            case TextureType::RGBA16bpp: {
                rgba* tex = raw2rgba(texData->mBuffer.data(), texData->mWidth, texData->mHeight, texData->mFormat.depth);
                for (int y = 0; y < texData->mHeight; y++) {
                    for (int x = 0; x < texData->mWidth; x++) {
                        size_t dstIdx = ((ulY + y) * atlasWidth + (ulX + x)) * 4;
                        size_t srcIdx = x + y * texData->mWidth;
                        textureData[dstIdx + 0] = tex[srcIdx].red;
                        textureData[dstIdx + 1] = tex[srcIdx].green;
                        textureData[dstIdx + 2] = tex[srcIdx].blue;
                        textureData[dstIdx + 3] = tex[srcIdx].alpha;
                    }
                }
                //memcpy(&textureData[t.second.x + (t.second.y * size)], tex, width * height * sizeof(rgba));
                free(tex);
                break;
            }
            case TextureType::Palette4bpp:
            case TextureType::Palette8bpp: {
                rgba* tex = ci2rgba32(texData->mBuffer.data(), palette->mBuffer.data(), texData->mWidth, texData->mHeight, texData->mFormat.depth);
                for (int y = 0; y < texData->mHeight; y++) {
                    for (int x = 0; x < texData->mWidth; x++) {
                        size_t dstIdx = ((ulY + y) * atlasWidth + (ulX + x)) * 4;
                        size_t srcIdx = x + y * texData->mWidth;
                        textureData[dstIdx + 0] = tex[srcIdx].red;
                        textureData[dstIdx + 1] = tex[srcIdx].green;
                        textureData[dstIdx + 2] = tex[srcIdx].blue;
                        textureData[dstIdx + 3] = tex[srcIdx].alpha;
                    }
                }
                //memcpy(&textureData[t.second.x + (t.second.y * size)], tex, width * height * sizeof(rgba));
                free(tex);
                break;
            }
            case TextureType::Grayscale4bpp:
            case TextureType::Grayscale8bpp: {
                ia* intermediate = raw2i(texData->mBuffer.data(), texData->mWidth, texData->mHeight, texData->mFormat.depth);
                rgba* tex = ia2rgba32(nullptr, intermediate, texData->mWidth, texData->mHeight);
                for (int y = 0; y < texData->mHeight; y++) {
                    for (int x = 0; x < texData->mWidth; x++) {
                        size_t dstIdx = ((ulY + y) * atlasWidth + (ulX + x)) * 4;
                        size_t srcIdx = x + y * texData->mWidth;
                        textureData[dstIdx + 0] = tex[srcIdx].red;
                        textureData[dstIdx + 1] = tex[srcIdx].green;
                        textureData[dstIdx + 2] = tex[srcIdx].blue;
                        textureData[dstIdx + 3] = tex[srcIdx].alpha;
                    }
                }
                //memcpy(&textureData[t.second.x + (t.second.y * size)], tex, width * height * sizeof(rgba));
                free(tex);
                free(intermediate);
                break;
            }
            case TextureType::GrayscaleAlpha4bpp:
            case TextureType::GrayscaleAlpha8bpp:
            case TextureType::GrayscaleAlpha16bpp:
            case TextureType::GrayscaleAlpha1bpp: {
                ia* intermediate = raw2ia(texData->mBuffer.data(), texData->mWidth, texData->mHeight, texData->mFormat.depth);
                rgba* tex = ia2rgba32(nullptr, intermediate, texData->mWidth, texData->mHeight);
                for (int y = 0; y < texData->mHeight; y++) {
                    for (int x = 0; x < texData->mWidth; x++) {
                        size_t dstIdx = ((ulY + y) * atlasWidth + (ulX + x)) * 4;
                        size_t srcIdx = x + y * texData->mWidth;
                        textureData[dstIdx + 0] = tex[srcIdx].red;
                        textureData[dstIdx + 1] = tex[srcIdx].green;
                        textureData[dstIdx + 2] = tex[srcIdx].blue;
                        textureData[dstIdx + 3] = tex[srcIdx].alpha;
                    }
                }
                //memcpy(&textureData[t.second.x + (t.second.y * size)], tex, width * height * sizeof(rgba));
                free(tex);
                free(intermediate);
                break;
            }
        }
    }
    // TODO remove this before shipping
#if 1
    unsigned char* outData;
    int sizeOut;
    rgba2png(&outData, &sizeOut, (rgba*)textureData, atlasWidth, atlasHeight);
    std::string pathA = Companion::Instance->GetCurrentFile();
    size_t lastSlashPos = pathA.find_last_of("/");
    pathA = pathA.substr(lastSlashPos + 1);
    const std::string path  = "/tmp/ATLASTEST/ATLAS_" + pathA + ".png";

    const int fd = open(path.c_str(), O_CREAT | O_RDWR, 0666);
    write(fd, outData, sizeOut);
    close(fd);
#endif
}

#include <immintrin.h>
__attribute__((target("lzcnt"))) static uint32_t RoundUpToPowerOf2(uint32_t value) {
#if 0
    if (!Lzcnt.IsSupported)
    {
        if (!X86Base.IsSupported)
        {
            --value;
            value |= value >> 1;
            value |= value >> 2;
            value |= value >> 4;
            value |= value >> 8;
            value |= value >> 16 /*0x10*/;
            return value + 1U;
        }
    }
#endif
    return (4294967296UL /*0x0100000000*/ >> __lzcnt32(value - 1U));
}

typedef struct AtlasSize {
    uint32_t width;
    uint32_t height;
} AtlasSize;

static uint32_t GetAtlasArea(const std::vector<std::shared_ptr<AtlasedTextures>>& textures) {
    return std::accumulate(textures.begin(), textures.end(), 0,
        [] (uint32_t size, const std::shared_ptr<AtlasedTextures>& b) {
            size += b->texData->mHeight * b->texData->mWidth;
            return size;
    });
}

std::optional<std::shared_ptr<IParsedData>> TextureAtlasFactory::parseLate(std::vector<ParseResultData>& parsedFiles) {
    std::shared_ptr<TextureAtlas> atlas = std::make_shared<TextureAtlas>();
    std::unordered_map<std::shared_ptr<AtlasedTextures>,Vec2i> imagePlacements;
    // Store all offsets listed as TLUTs so they aren't included in the atlas
    std::set<uint32_t> tlutOffsets;
    for (auto pf : parsedFiles) {
        if (pf.type == "TEXTURE") {

            AtlasedTextures tex;
            tex.texData = static_pointer_cast<TextureData>(pf.data.value());
            tex.texOffset = pf.GetOffset() & 0x00FFFFFF; // The segment was added but tlutOffsets doesn't use it
            if (tex.texData->mFormat.type == TextureType::Palette8bpp ||  tex.texData->mFormat.type == TextureType::Palette4bpp) {
                auto curSegNum = Companion::Instance->GetCurrSegmentNumber();
                const auto tlutOffset = pf.node["tlut"];
                if (tlutOffset) {
                    uint32_t tlutOffset32 = tlutOffset.as<uint32_t>();
                    auto tlutData = Companion::Instance->GetParseDataByAddr(curSegNum << 24 | tlutOffset.as<uint32_t>());
                    tlutOffsets.insert(tlutOffset32);
                    tex.palette = std::static_pointer_cast<TextureData>(tlutData->data.value());
                } else if (pf.node["tlut_symbol"]) {
                    auto tlutData = Companion::Instance->GetParseDataBySymbol(pf.node["tlut_symbol"].as<std::string>());
                    tex.palette = std::static_pointer_cast<TextureData>(tlutData->data.value());
                }
                else if (pf.node["external_tlut"]) {
                    const auto externalTlutFile = GetSafeNode<std::string>(pf.node, "external_tlut");
                    const auto externalTlutOffset = GetSafeNode<uint32_t>(pf.node, "external_tlut_offset");
                    const auto externalSeg = Companion::Instance->GetFileSegmentNumber(externalTlutFile);

                    auto tlutData = Companion::Instance->GetParseDataByAddr(externalSeg << 24 | externalTlutOffset);
                    tex.palette = std::static_pointer_cast<TextureData>(tlutData->data.value());
                }
            }
            atlas->textures.push_back(std::make_shared<AtlasedTextures>(tex));
        }
    }

    std::erase_if(atlas->textures,
                  [tlutOffsets](const std::shared_ptr<AtlasedTextures>& t) { return tlutOffsets.contains(t->texOffset); });
    // Atlases with only 1 texture will only waste space.
    if (atlas->textures.empty() || atlas->textures.size() == 1)
        return std::nullopt;

    //std::sort(atlas->textures.begin(), atlas->textures.end(),
    //    [](const std::shared_ptr<AtlasedTextures>& a, const std::shared_ptr<AtlasedTextures>& b) {
    //   return (a->texData->mHeight * a->texData->mWidth > b->texData->mWidth * b->texData->mHeight);
    //});

    auto sizeI = GetAtlasArea(atlas->textures);
    AtlasSize size;
    sizeI = sqrtf(sizeI);
    size.height = RoundUpToPowerOf2(sizeI);
    size.width = RoundUpToPowerOf2(sizeI);

    while (!AttemptToPackImages(size.width, size.height, atlas->textures, imagePlacements)) {
        // if (size.width < 512) {
        size.width *= 2;
        //                if (AttemptToPackImages(size.height, size.width, atlas->textures, imagePlacements)) break;
        //}
        if (size.height < 512) {
            size.height *= 2;
            //              if (AttemptToPackImages(size.height, size.width, atlas->textures, imagePlacements)) break;
        } else {
            //   throw new std::runtime_error("Atlas needs to be larger than 512 x 512. This probably won't work with
            //   F3D.");
        }
    }
    BuildAtlasBinary(imagePlacements, size.width, size.height);
    return atlas;
}
