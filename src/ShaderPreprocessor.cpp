#include "ShaderPreprocessor.h"
#include "Logger.h" // For LOG_ERROR
#include "GeneratedShaderLibraries.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <algorithm>
#include <iomanip>

// Helper function to read a file's content
static std::string readFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

ShaderPreprocessor::ShaderPreprocessor() {
    // Default empty onMessage callback
    onMessage = [](const std::string& msg) { LOG_ERROR("ShaderPreprocessor: {}", msg); };

    // Initialize embedded libraries
    EmbeddedLibraries::initialize();
}

ShaderPreprocessor::PreprocessResult ShaderPreprocessor::preprocess(const std::string& filePath, RenderScaleMode scaleMode) {
    PreprocessResult result;
    std::vector<std::string> includeStack;
    std::set<std::string> uniqueIncludedFiles;
    int currentLine = 1;
    std::string currentGroup = "";
    result.source = preprocessRecursive(filePath, includeStack, uniqueIncludedFiles, result.switchFlags, result.sliders, result.uniformRanges, result.labels, result.lineMappings, result.groupChanges, currentGroup, currentLine);

    for (const auto& file : uniqueIncludedFiles) {
        result.includedFiles.push_back(file);
    }
    
    // Conditional Chunk Logic Injection
    if ((scaleMode == RenderScaleMode::Chunk || scaleMode == RenderScaleMode::Auto) && filePath.find(".frag") != std::string::npos) {
        std::string chunkUniforms = R"(
// Chunk rendering uniforms
uniform bool u_progressive_fill;
uniform int u_render_phase;
uniform float u_renderChunkFactor;
uniform float u_time_offset;
uniform int u_chunk_stride;

bool shouldDiscard() {
    if (!u_progressive_fill) return false;
    // Use 2x2 pixel blocks to preserve quad efficiency (derivatives)
    // dividing gl_FragCoord by 2 ensures that a 2x2 pixel quad falls into the same "chunk"
    ivec2 coord = ivec2(gl_FragCoord.xy) / 2;
    // Stride-based stipple pattern for variable density
    int phase = (coord.x % u_chunk_stride) + (coord.y % u_chunk_stride) * u_chunk_stride;
    return phase != u_render_phase;
}
)";
        
        // Insert uniforms and helper function after version directive
        int insertedLines = std::count(chunkUniforms.begin(), chunkUniforms.end(), '\n');
        size_t versionPos = result.source.find("#version");
        int insertionLine = 1;

        if (versionPos != std::string::npos) {
            size_t eolPos = result.source.find('\n', versionPos);
            if (eolPos != std::string::npos) {
                result.source.insert(eolPos + 1, chunkUniforms);
                insertionLine = 2; // Inserted after line 1
            }
        } else {
            result.source.insert(0, chunkUniforms);
            insertionLine = 1;
        }

        // Shift metadata after the injection point and insert new mappings
        auto shiftMetadata = [&](int fromLine, int delta, const std::string& virtualPath, int startFileLine) {
            for (auto& range : result.uniformRanges) {
                if (range.line >= fromLine) range.line += delta;
            }
            for (auto& change : result.groupChanges) {
                if (change.line >= fromLine) change.line += delta;
            }
            for (auto& mapping : result.lineMappings) {
                if (mapping.preprocessedLine >= fromLine) mapping.preprocessedLine += delta;
            }

            // Insert new mappings for the injected code
            for (int i = 0; i < delta; ++i) {
                result.lineMappings.push_back({fromLine + i, virtualPath, startFileLine + i});
            }
            // Keep mappings sorted by preprocessedLine
            std::sort(result.lineMappings.begin(), result.lineMappings.end(), [](const LineMapping& a, const LineMapping& b) {
                return a.preprocessedLine < b.preprocessedLine;
            });
        };

        const std::string chunkVirtualPath = "<internal:chunk_logic>";
        shiftMetadata(insertionLine, insertedLines, chunkVirtualPath, 1);
        
        // Insert discard check at start of main
        std::regex mainRegex(R"(void\s+main\s*\(\s*\)\s*\{)");
        std::smatch matches;
        if (std::regex_search(result.source, matches, mainRegex)) {
            size_t mainPos = matches.position() + matches.length();
            
            // Find which line main is on to shift mappings after it
            int mainLine = std::count(result.source.begin(), result.source.begin() + mainPos, '\n') + 1;
            
            std::string discardLogic = "\n    if (shouldDiscard()) discard;\n";
            int discardLines = std::count(discardLogic.begin(), discardLogic.end(), '\n');
            
            result.source.insert(mainPos, discardLogic);
            shiftMetadata(mainLine, discardLines, chunkVirtualPath, insertedLines + 1);
        }
    }
    
    return result;
}

std::string ShaderPreprocessor::preprocessRecursive(const std::string& filePath,
                                                    std::vector<std::string>& includeStack,
                                                    std::set<std::string>& uniqueIncludedFiles,
                                                    std::vector<SwitchInfo>& switchFlags,
                                                    std::vector<SliderInfo>& sliders,
                                                    std::vector<UniformRange>& uniformRanges,
                                                    std::vector<LabelInfo>& labels,
                                                    std::vector<LineMapping>& lineMappings,
                                                    std::vector<GroupChange>& groupChanges,
                                                    std::string& currentGroup,
                                                    int& currentLine) {
    LOG_DEBUG("Preprocessing file: {}", filePath);
    // Check for include loops
    if (std::find(includeStack.begin(), includeStack.end(), filePath) != includeStack.end()) {
        std::string errorMsg = "Include loop detected: " + filePath;
        if (onMessage) onMessage(errorMsg);
        return "#error " + errorMsg + "\n";
    }

    includeStack.push_back(filePath);
    uniqueIncludedFiles.insert(filePath);

    std::string source = readFileContent(filePath);
    if (source.empty()) {
        includeStack.pop_back();
        std::string errorMsg = "Failed to read file: " + filePath;
        if (onMessage) onMessage(errorMsg);
        return "#error " + errorMsg + "\n";
    }

    std::string result = preprocessSource(source, filePath, includeStack, uniqueIncludedFiles, switchFlags, sliders, uniformRanges, labels, lineMappings, groupChanges, currentGroup, currentLine);

    includeStack.pop_back();
    return result;
}

std::string ShaderPreprocessor::preprocessSource(const std::string& source,
                                                 const std::string& filePath,
                                                 std::vector<std::string>& includeStack,
                                                 std::set<std::string>& uniqueIncludedFiles,
                                                 std::vector<SwitchInfo>& switchFlags,
                                                 std::vector<SliderInfo>& sliders,
                                                 std::vector<UniformRange>& uniformRanges,
                                                 std::vector<LabelInfo>& labels,
                                                 std::vector<LineMapping>& lineMappings,
                                                 std::vector<GroupChange>& groupChanges,
                                                 std::string& currentGroup,
                                                 int& currentLine) {
    LOG_DEBUG("Processing source for: {}", filePath);

    struct SourceLine {
        std::string content;
        int originalLineNumber;
    };

    std::vector<SourceLine> sourceLines;
    std::stringstream ss(source);
    std::string line;
    int lineCounter = 0;
    while (std::getline(ss, line)) {
        sourceLines.push_back({line, ++lineCounter});
    }

    std::regex versionRegex(R"x(\s*#\s*version\s+\d+\s+\w*)x");
    SourceLine versionDirective = {"", 0};

    // Extract #version directive if present
    for (auto it = sourceLines.begin(); it != sourceLines.end(); ) {
        if (std::regex_match(it->content, versionRegex) && versionDirective.content.empty()) {
            versionDirective = *it;
            it = sourceLines.erase(it);
        } else {
            ++it;
        }
    }

    std::stringstream preprocessedSource;

    // Process version directive first
    if (!versionDirective.content.empty()) {
        preprocessedSource << versionDirective.content << "\n";
        lineMappings.push_back({currentLine++, filePath, versionDirective.originalLineNumber});
    }
    
    std::regex includeRegex(R"x(#pragma\s+include\s*(?:\(\s*)?(?:<([^>]+)>|"([^"]+)"|([^\s\)"<]+))(?:\s*\))?)x");
    std::regex switchRegex(R"x(#pragma\s+switch\s*\(\s*([a-zA-Z0-9_]+)\s*(?:,\s*(true|false|on|off|0|1))?\s*(?:,\s*["']([^"']+)["'])?\s*(?:,\s*["']([^"']+)["'])?\s*\))x");
    std::regex sliderRegex(R"x(#pragma\s+slider\s*\(\s*([a-zA-Z0-9_]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*(?:,\s*([-+]?[0-9]*\.?[0-9]+))?\s*(?:,\s*["']([^"']+)["'])?\s*\))x");
    std::regex rangeRegex(R"x(#pragma\s+range\s*\(\s*([a-zA-Z0-9_]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*(?:,\s*([-+]?[0-9]*\.?[0-9]+))?\s*(?:,\s*["']([^"']+)["'])?\s*\))x");
    std::regex rangePositionalRegex(R"x(#pragma\s+range\s*\(\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*(?:,\s*([-+]?[0-9]*\.?[0-9]+))?\s*(?:,\s*["']([^"']+)["'])?\s*\))x");
    std::regex labelRegex(R"x(#pragma\s+label\s*\(\s*([a-zA-Z0-9_]+)\s*,\s*["']([^"']+)["']\s*\))x");
    std::regex groupRegex(R"x(#pragma\s+group\s*\(\s*["']([^"']+)["']\s*\))x");
    std::regex endGroupRegex(R"x(#pragma\s+endgroup\s*(?:\(\s*\))?)x");

    for (const auto& sourceLine : sourceLines) {
        std::string currentLineContent = sourceLine.content;
        int fileLineNumber = sourceLine.originalLineNumber;
        std::smatch matches;

        // Scan for group start
        if (std::regex_search(currentLineContent, matches, groupRegex)) {
            if (matches.size() >= 2) {
                currentGroup = matches[1].str();
                groupChanges.push_back({currentLine, currentGroup});
            }
        }
        
        // Scan for group end
        if (std::regex_search(currentLineContent, matches, endGroupRegex)) {
            currentGroup = "";
            groupChanges.push_back({currentLine, ""});
        }

        // Scan for all switches on the line
        auto switchBegin = std::sregex_iterator(currentLineContent.begin(), currentLineContent.end(), switchRegex);
        auto switchEnd = std::sregex_iterator();
        
        for (std::sregex_iterator i = switchBegin; i != switchEnd; ++i) {
            std::smatch match = *i;
            if (match.size() >= 2) {
                SwitchInfo sw;
                sw.name = match[1].str();
                
                if (match.size() >= 3 && !match[2].str().empty()) {
                    std::string defaultStr = match[2].str();
                    sw.defaultValue = (defaultStr == "true" || defaultStr == "on" || defaultStr == "1");
                }
                
                if (match.size() >= 4 && !match[3].str().empty()) {
                    sw.label = match[3].str();
                }

                if (match.size() >= 5 && !match[4].str().empty()) {
                    sw.labelOn = match[4].str();
                }
                
                sw.group = currentGroup;
                switchFlags.push_back(sw);
            }
        }

        // Scan for sliders
        auto sliderBegin = std::sregex_iterator(currentLineContent.begin(), currentLineContent.end(), sliderRegex);
        for (std::sregex_iterator i = sliderBegin; i != switchEnd; ++i) {
            std::smatch match = *i;
            if (match.size() >= 4) {
                SliderInfo sl;
                sl.name = match[1].str();
                sl.min = std::stof(match[2].str());
                sl.max = std::stof(match[3].str());

                if (match.size() >= 5 && !match[4].str().empty()) {
                    sl.defaultValue = std::stof(match[4].str());
                    sl.hasDefaultValue = true;
                }

                if (match.size() >= 6 && !match[5].str().empty()) {
                    sl.label = match[5].str();
                }
                sl.group = currentGroup;
                sliders.push_back(sl);
            }
        }
        // Scan for named ranges
        auto rangeBegin = std::sregex_iterator(currentLineContent.begin(), currentLineContent.end(), rangeRegex);
        for (std::sregex_iterator i = rangeBegin; i != switchEnd; ++i) {
            std::smatch match = *i;
            if (match.size() >= 4) {
                UniformRange r;
                r.name = match[1].str();
                r.min = std::stof(match[2].str());
                r.max = std::stof(match[3].str());

                if (match.size() >= 5 && !match[4].str().empty()) {
                    r.defaultValue = std::stof(match[4].str());
                    r.hasDefaultValue = true;
                }

                if (match.size() >= 6 && !match[5].str().empty()) {
                    r.label = match[5].str();
                }
                uniformRanges.push_back(r);
            }
        }
        
        // Scan for positional ranges (min, max [, defaultValue [, "label"]])
        auto rangePosBegin = std::sregex_iterator(currentLineContent.begin(), currentLineContent.end(), rangePositionalRegex);
        for (std::sregex_iterator i = rangePosBegin; i != switchEnd; ++i) {
            std::smatch match = *i;
            if (match.size() >= 3) {
                UniformRange r;
                r.min = std::stof(match[1].str());
                r.max = std::stof(match[2].str());

                if (match.size() >= 4 && !match[3].str().empty()) {
                    r.defaultValue = std::stof(match[3].str());
                    r.hasDefaultValue = true;
                }

                if (match.size() >= 5 && !match[4].str().empty()) {
                    r.label = match[4].str();
                }
                r.line = currentLine;
                uniformRanges.push_back(r);
            }
        }

        // Scan for labels
        auto labelBegin = std::sregex_iterator(currentLineContent.begin(), currentLineContent.end(), labelRegex);
        for (std::sregex_iterator i = labelBegin; i != switchEnd; ++i) {
            std::smatch match = *i;
            if (match.size() == 3) {
                labels.push_back({match[1].str(), match[2].str()});
            }
        }

        if (std::regex_search(currentLineContent, matches, includeRegex)) {
            if (matches.size() >= 4) {
                std::string libInclude = matches[1].str();
                std::string quoteInclude = matches[2].str();
                std::string bareInclude = matches[3].str();
                
                std::string includeFileName;
                if (!libInclude.empty()) includeFileName = libInclude;
                else if (!quoteInclude.empty()) includeFileName = quoteInclude;
                else includeFileName = bareInclude;

                std::string resourceName = includeFileName;
                bool isLibPrefix = false;
                if (resourceName.rfind("libs/", 0) == 0) {
                    resourceName = resourceName.substr(5);
                    isLibPrefix = true;
                } else if (resourceName.rfind("lib/", 0) == 0) {
                    resourceName = resourceName.substr(4);
                    isLibPrefix = true;
                }
                
                bool isLibraryInclude = isLibPrefix || !libInclude.empty();
                std::string includedContent;
                bool found = false;

                if (isLibraryInclude) {
                    // 1. Search locally in project libs folder
                    // We assume project structure: [root]/shaders/ and [root]/libs/
                    std::filesystem::path currentFile(filePath);
                    std::filesystem::path projectRoot = currentFile.parent_path();
                    if (projectRoot.filename() == "shaders") {
                        projectRoot = projectRoot.parent_path();
                    }
                    
                    std::filesystem::path localLibPath = projectRoot / "libs" / resourceName;
                    if (std::filesystem::exists(localLibPath)) {
                        includedContent = preprocessRecursive(localLibPath.string(), includeStack, uniqueIncludedFiles, switchFlags, sliders, uniformRanges, labels, lineMappings, groupChanges, currentGroup, currentLine);
                        found = true;
                    }

                    if (!found) {
                        // 2. Search in bundled shaders (embedded)
                        auto it = EmbeddedLibraries::g_libs.find(resourceName);
                        if (it == EmbeddedLibraries::g_libs.end() && !libInclude.empty()) {
                            // Also try with prefix if <lib> was used but not found stripped
                            it = EmbeddedLibraries::g_libs.find(libInclude);
                        }

                        if (it != EmbeddedLibraries::g_libs.end()) {
                            std::string libContent(it->second.first, it->second.second);
                            std::string embeddedName = "embedded:" + resourceName;
                            if (std::find(includeStack.begin(), includeStack.end(), embeddedName) != includeStack.end()) {
                                std::string errorMsg = "Include loop detected: " + embeddedName;
                                if (onMessage) onMessage(errorMsg);
                                includedContent = "#error " + errorMsg + "\n";
                            } else {
                                includeStack.push_back(embeddedName);
                                includedContent = preprocessSource(libContent, embeddedName, includeStack, uniqueIncludedFiles, switchFlags, sliders, uniformRanges, labels, lineMappings, groupChanges, currentGroup, currentLine);
                                includeStack.pop_back();
                            }
                            found = true;
                        }
                    }
                } else {
                    // 3. Search locally in same folder as the shader (yyyy)
                    std::filesystem::path currentDir = std::filesystem::path(filePath).parent_path();
                    std::filesystem::path includePath = currentDir / includeFileName;
                    
                    if (std::filesystem::exists(includePath)) {
                        includedContent = preprocessRecursive(includePath.string(), includeStack, uniqueIncludedFiles, switchFlags, sliders, uniformRanges, labels, lineMappings, groupChanges, currentGroup, currentLine);
                        found = true;
                    }
                }

                if (found) {
                    preprocessedSource << includedContent;
                } else {
                    std::string errorMsg = "Include not found: " + includeFileName;
                    if (onMessage) onMessage(errorMsg);
                    preprocessedSource << "#error " + errorMsg + "\n";
                    lineMappings.push_back({currentLine++, filePath, fileLineNumber});
                }
            } else {
                std::string errorMsg = "Invalid include directive: " + currentLineContent;
                if (onMessage) onMessage(errorMsg);
                preprocessedSource << "#error " + errorMsg + "\n";
                lineMappings.push_back({currentLine++, filePath, fileLineNumber});
            }
        } else if (std::sregex_iterator(currentLineContent.begin(), currentLineContent.end(), switchRegex) != switchEnd ||
                   std::sregex_iterator(currentLineContent.begin(), currentLineContent.end(), sliderRegex) != switchEnd ||
                   std::sregex_iterator(currentLineContent.begin(), currentLineContent.end(), rangeRegex) != switchEnd ||
                   std::sregex_iterator(currentLineContent.begin(), currentLineContent.end(), rangePositionalRegex) != switchEnd ||
                   std::sregex_iterator(currentLineContent.begin(), currentLineContent.end(), labelRegex) != switchEnd ||
                   std::regex_search(currentLineContent, groupRegex) ||
                   std::regex_search(currentLineContent, endGroupRegex)) {
            // Line contained pragmas that we parsed above, so we consume it.
        } else {
            preprocessedSource << currentLineContent << "\n";
            lineMappings.push_back({currentLine++, filePath, fileLineNumber});
        }
    }

    return preprocessedSource.str();
}

std::string ShaderPreprocessor::preprocessAndBake(const std::string& filePath,
                                                  const std::map<std::string, std::vector<float>>& uniforms,
                                                  const std::map<std::string, bool>& switches,
                                                  const std::map<std::string, int>& sliders,
                                                  int xres, int yres,
                                                  RenderScaleMode scaleMode) {
    // 1. Run the standard preprocessor
    PreprocessResult result = preprocess(filePath, scaleMode);
    
    if (result.source.empty() || result.source.find("#error") != std::string::npos) {
        return result.source;
    }
    
    // 2. Insert resolution and define injections after the #version directive
    std::string injected;
    
    // Inject resolution defines
    injected += "#define XRES " + std::to_string(xres) + "\n";
    injected += "#define YRES " + std::to_string(yres) + "\n";
    
    // Inject switch defines
    for (const auto& sw : result.switchFlags) {
        bool enabled = sw.defaultValue;
        if (switches.find(sw.name) != switches.end()) {
            enabled = switches.at(sw.name);
        }
        if (enabled) {
            injected += "#define " + sw.name + "\n";
        }
    }
    
    // Inject slider defines
    for (const auto& sl : result.sliders) {
        int value = sl.defaultValue;
        if (sliders.find(sl.name) != sliders.end()) {
            value = sliders.at(sl.name);
        }
        injected += "#define " + sl.name + " " + std::to_string(value) + "\n";
    }
    
    std::string finalSource = result.source;
    size_t versionPos = finalSource.find("#version");
    size_t insertPos = 0;
    if (versionPos != std::string::npos) {
        size_t eolPos = finalSource.find('\n', versionPos);
        if (eolPos != std::string::npos) {
            insertPos = eolPos + 1;
        }
    }
    finalSource.insert(insertPos, injected);
    
    // Helper to format float values
    auto formatFloat = [](float f) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << f;
        std::string s = ss.str();
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') {
            s += '0';
        }
        return s;
    };
    
    // 3. Replace uniform declarations in the source
    std::regex uniformRegex(R"(uniform\s+(float|vec2|vec3|vec4|samplerBuffer)\s+([a-zA-Z0-9_]+)(?:\[(\d+)\])?\s*;)");
    std::string bakedSource;
    auto lastPos = finalSource.cbegin();
    auto begin = std::sregex_iterator(finalSource.cbegin(), finalSource.cend(), uniformRegex);
    auto end = std::sregex_iterator();
    
    for (auto it = begin; it != end; ++it) {
        std::smatch match = *it;
        bakedSource.append(lastPos, finalSource.cbegin() + match.position());
        
        std::string typeStr = match[1].str();
        std::string nameStr = match[2].str();
        
        if (nameStr == "iTime" || nameStr == "u_time") {
            bakedSource.append(match.str());
        } else if (typeStr == "samplerBuffer") {
            bakedSource.append(match.str());
        } else if (nameStr == "iResolution" || nameStr == "u_resolution") {
            if (typeStr == "vec3") {
                bakedSource.append("const vec3 " + nameStr + " = vec3(XRES, YRES, 1.0);");
            } else {
                bakedSource.append("const vec2 " + nameStr + " = vec2(XRES, YRES);");
            }
        } else {
            // Find value
            std::vector<float> val = {0.0f, 0.0f, 0.0f, 0.0f};
            if (uniforms.find(nameStr) != uniforms.end()) {
                val = uniforms.at(nameStr);
            } else {
                // Try to find default from ranges
                for (const auto& r : result.uniformRanges) {
                    if (r.name == nameStr && r.hasDefaultValue) {
                        val[0] = val[1] = val[2] = val[3] = r.defaultValue;
                        break;
                    }
                }
            }
            
            if (typeStr == "float") {
                bakedSource.append("const float " + nameStr + " = " + formatFloat(val[0]) + ";");
            } else if (typeStr == "vec2") {
                bakedSource.append("const vec2 " + nameStr + " = vec2(" + formatFloat(val[0]) + ", " + formatFloat(val[1]) + ");");
            } else if (typeStr == "vec3") {
                bakedSource.append("const vec3 " + nameStr + " = vec3(" + formatFloat(val[0]) + ", " + formatFloat(val[1]) + ", " + formatFloat(val[2]) + ");");
            } else if (typeStr == "vec4") {
                bakedSource.append("const vec4 " + nameStr + " = vec4(" + formatFloat(val[0]) + ", " + formatFloat(val[1]) + ", " + formatFloat(val[2]) + ", " + formatFloat(val[3]) + ");");
            } else {
                bakedSource.append(match.str());
            }
        }
        lastPos = match.suffix().first;
    }
    bakedSource.append(lastPos, finalSource.cend());
    
    return bakedSource;
}