#pragma once

#include "BaseFactory.h"
#include "utils/TextureUtils.h"
#include "TextureFactory.h"

struct AtlasedTextures {
    std::shared_ptr<TextureData> texData = nullptr;
    std::shared_ptr<TextureData> palette = nullptr;
    uint32_t texOffset;
    bool splitTlut = false;
};

class TextureAtlas : public IParsedData {
public:
    std::vector<std::shared_ptr<AtlasedTextures>> textures;
    std::unique_ptr<uint8_t[]> mAtlasData;
    uint32_t mAtlasWidth;
    uint32_t mAtlasHeight;
};

class TextureAtlasHeaderExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class TextureAtlasCodeExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class TextureAtlasBinaryExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class TextureAtlasModdingExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class TextureAtlasFactory : public BaseFactory {
public:
    std::optional<std::shared_ptr<IParsedData>> parse(std::vector<uint8_t>& buffer, YAML::Node& data) override;
    std::optional<std::shared_ptr<IParsedData>> parseLate(YAML::Node& node, std::vector<ParseResultData>& parsedFiles);
    inline std::unordered_map<ExportType, std::shared_ptr<BaseExporter>> GetExporters() override {
        return {
            REGISTER(Header, TextureAtlasHeaderExporter)
            REGISTER(Binary, TextureAtlasBinaryExporter)
            REGISTER(Code, TextureAtlasCodeExporter)
            REGISTER(Modding, TextureAtlasModdingExporter)
        };
    }
};
