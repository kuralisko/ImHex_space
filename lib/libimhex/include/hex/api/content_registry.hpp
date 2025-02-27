#pragma once

#include <hex.hpp>
#include <hex/helpers/concepts.hpp>

#include <pl/pattern_language.hpp>
#include <hex/api/imhex_api.hpp>
#include <hex/api/event.hpp>

#include <functional>
#include <map>
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json_fwd.hpp>

using ImGuiDataType = int;
using ImGuiInputTextFlags = int;

namespace hex {

    class View;
    class LanguageDefinition;

    namespace dp {
        class Node;
    }
    namespace prv {
        class Provider;
    }

    /*
        The Content Registry is the heart of all features in ImHex that are in some way extendable by Plugins.
        It allows you to add/register new content that will be picked up and used by the ImHex core or by other
        plugins when needed.
    */
    namespace ContentRegistry {

        /* Settings Registry. Allows adding of new entries into the ImHex preferences window. */
        namespace Settings {
            using Callback = std::function<bool(const std::string &, nlohmann::json &)>;

            struct Entry {
                std::string name;
                bool requiresRestart;
                Callback callback;
            };

            struct Category {
                std::string name;
                size_t slot = 0;

                bool operator<(const Category &other) const {
                    return name < other.name;
                }

                explicit operator const std::string &() const {
                    return name;
                }
            };

            void load();
            void store();
            void clear();

            void add(const std::string &unlocalizedCategory, const std::string &unlocalizedName, i64 defaultValue, const Callback &callback, bool requiresRestart = false);
            void add(const std::string &unlocalizedCategory, const std::string &unlocalizedName, const std::string &defaultValue, const Callback &callback, bool requiresRestart = false);
            void add(const std::string &unlocalizedCategory, const std::string &unlocalizedName, const std::vector<std::string> &defaultValue, const Callback &callback, bool requiresRestart = false);

            void addCategoryDescription(const std::string &unlocalizedCategory, const std::string &unlocalizedCategoryDescription);

            void write(const std::string &unlocalizedCategory, const std::string &unlocalizedName, i64 value);
            void write(const std::string &unlocalizedCategory, const std::string &unlocalizedName, const std::string &value);
            void write(const std::string &unlocalizedCategory, const std::string &unlocalizedName, const std::vector<std::string> &value);

            i64 read(const std::string &unlocalizedCategory, const std::string &unlocalizedName, i64 defaultValue);
            std::string read(const std::string &unlocalizedCategory, const std::string &unlocalizedName, const std::string &defaultValue);
            std::vector<std::string> read(const std::string &unlocalizedCategory, const std::string &unlocalizedName, const std::vector<std::string> &defaultValue = {});

            std::map<Category, std::vector<Entry>> &getEntries();
            std::map<std::string, std::string> &getCategoryDescriptions();
            nlohmann::json getSetting(const std::string &unlocalizedCategory, const std::string &unlocalizedName);
            nlohmann::json &getSettingsData();
        }

        /* Command Palette Command Registry. Allows adding of new commands to the command palette */
        namespace CommandPaletteCommands {

            enum class Type : u32
            {
                SymbolCommand,
                KeywordCommand
            };

            using DisplayCallback = std::function<std::string(std::string)>;
            using ExecuteCallback = std::function<void(std::string)>;

            struct Entry {
                Type type;
                std::string command;
                std::string unlocalizedDescription;
                DisplayCallback displayCallback;
                ExecuteCallback executeCallback;
            };

            void add(
                Type type,
                const std::string &command,
                const std::string &unlocalizedDescription,
                const DisplayCallback &displayCallback,
                const ExecuteCallback &executeCallback = [](auto) {});
            std::vector<Entry> &getEntries();
        }

        /* Pattern Language Function Registry. Allows adding of new functions that may be used inside the pattern language */
        namespace PatternLanguage {

            namespace impl {

                struct FunctionDefinition {
                    pl::api::Namespace ns;
                    std::string name;

                    pl::api::FunctionParameterCount parameterCount;
                    pl::api::FunctionCallback callback;

                    bool dangerous;
                };

            }

            void configureRuntime(pl::PatternLanguage &runtime, prv::Provider *provider);

            void addPragma(const std::string &name, const pl::api::PragmaHandler &handler);

            void addFunction(const pl::api::Namespace &ns, const std::string &name, pl::api::FunctionParameterCount parameterCount, const pl::api::FunctionCallback &func);
            void addDangerousFunction(const pl::api::Namespace &ns, const std::string &name, pl::api::FunctionParameterCount parameterCount, const pl::api::FunctionCallback &func);

            std::map<std::string, pl::api::PragmaHandler> &getPragmas();
            std::vector<impl::FunctionDefinition> &getFunctions();

        }

        /* View Registry. Allows adding of new windows */
        namespace Views {

            namespace impl {

                void add(View *view);

            }

            template<std::derived_from<View> T, typename... Args>
            void add(Args &&...args) {
                return impl::add(new T(std::forward<Args>(args)...));
            }

            std::map<std::string, View *> &getEntries();

            View *getViewByName(const std::string &unlocalizedName);

        }

        /* Tools Registry. Allows adding new entries to the tools window */
        namespace Tools {

            namespace impl {

                using Callback = std::function<void()>;

                struct Entry {
                    std::string name;
                    Callback function;
                    bool detached;
                };

            }

            void add(const std::string &unlocalizedName, const impl::Callback &function);

            std::vector<impl::Entry> &getEntries();
        }

        /* Data Inspector Registry. Allows adding of new types to the data inspector */
        namespace DataInspector {

            enum class NumberDisplayStyle
            {
                Decimal,
                Hexadecimal,
                Octal
            };

            namespace impl {

                using DisplayFunction   = std::function<std::string()>;
                using EditingFunction   = std::function<std::vector<u8>(std::string, std::endian)>;
                using GeneratorFunction = std::function<DisplayFunction(const std::vector<u8> &, std::endian, NumberDisplayStyle)>;

                struct Entry {
                    std::string unlocalizedName;
                    size_t requiredSize;
                    size_t maxSize;
                    impl::GeneratorFunction generatorFunction;
                    std::optional<impl::EditingFunction> editingFunction;
                };

            }

            void add(const std::string &unlocalizedName, size_t requiredSize, impl::GeneratorFunction displayGeneratorFunction, std::optional<impl::EditingFunction> editingFunction = std::nullopt);
            void add(const std::string &unlocalizedName, size_t requiredSize, size_t maxSize, impl::GeneratorFunction displayGeneratorFunction, std::optional<impl::EditingFunction> editingFunction = std::nullopt);

            std::vector<impl::Entry> &getEntries();
        }

        /* Data Processor Node Registry. Allows adding new processor nodes to be used in the data processor */
        namespace DataProcessorNode {

            namespace impl {

                using CreatorFunction = std::function<std::unique_ptr<dp::Node>()>;

                struct Entry {
                    std::string category;
                    std::string name;
                    CreatorFunction creatorFunction;
                };

                void add(const Entry &entry);

            }


            template<std::derived_from<dp::Node> T, typename... Args>
            void add(const std::string &unlocalizedCategory, const std::string &unlocalizedName, Args &&...args) {
                add(impl::Entry {
                    unlocalizedCategory.c_str(),
                    unlocalizedName.c_str(),
                    [=] {
                        auto node = std::make_unique<T>(std::forward<Args>(args)...);
                        node->setUnlocalizedName(unlocalizedName);
                        return node;
                    }
                });
            }

            void addSeparator();

            std::vector<impl::Entry> &getEntries();

        }

        /* Language Registry. Allows together with the LangEntry class and the _lang user defined literal to add new languages */
        namespace Language {
            void addLocalization(const nlohmann::json &data);

            std::map<std::string, std::string> &getLanguages();
            std::map<std::string, std::vector<LanguageDefinition>> &getLanguageDefinitions();
        }

        /* Interface Registry. Allows adding new items to various interfaces */
        namespace Interface {

            namespace impl {

                using DrawCallback   = std::function<void()>;
                using LayoutFunction = std::function<void(u32)>;
                using ClickCallback  = std::function<void()>;

                struct Layout {
                    std::string unlocalizedName;
                    LayoutFunction callback;
                };

                struct MainMenuItem {
                    std::string unlocalizedName;
                };

                struct MenuItem {
                    std::string unlocalizedName;
                    DrawCallback callback;
                };

                struct SidebarItem {
                    std::string icon;
                    DrawCallback callback;
                };

                struct TitleBarButton {
                    std::string icon;
                    std::string unlocalizedTooltip;
                    ClickCallback callback;
                };

            }

            void registerMainMenuItem(const std::string &unlocalizedName, u32 priority);
            void addMenuItem(const std::string &unlocalizedMainMenuName, u32 priority, const impl::DrawCallback &function);

            void addWelcomeScreenEntry(const impl::DrawCallback &function);
            void addFooterItem(const impl::DrawCallback &function);
            void addToolbarItem(const impl::DrawCallback &function);
            void addSidebarItem(const std::string &icon, const impl::DrawCallback &function);
            void addTitleBarButton(const std::string &icon, const std::string &unlocalizedTooltip, const impl::ClickCallback &function);

            void addLayout(const std::string &unlocalizedName, const impl::LayoutFunction &function);

            std::multimap<u32, impl::MainMenuItem> &getMainMenuItems();
            std::multimap<u32, impl::MenuItem> &getMenuItems();

            std::vector<impl::DrawCallback> &getWelcomeScreenEntries();
            std::vector<impl::DrawCallback> &getFooterItems();
            std::vector<impl::DrawCallback> &getToolbarItems();
            std::vector<impl::SidebarItem> &getSidebarItems();
            std::vector<impl::TitleBarButton> &getTitleBarButtons();

            std::vector<impl::Layout> &getLayouts();
        }

        /* Provider Registry. Allows adding new data providers to be created from the UI */
        namespace Provider {

            namespace impl {

                void addProviderName(const std::string &unlocalizedName);

            }

            template<std::derived_from<hex::prv::Provider> T>
            void add(bool addToList = true) {
                auto typeName = T().getTypeName();

                (void)EventManager::subscribe<RequestCreateProvider>([expectedName = typeName](const std::string &name, bool skipLoadInterface, hex::prv::Provider **provider) {
                    if (name != expectedName) return;

                    prv::Provider *newProvider = new T();

                    hex::ImHexApi::Provider::add(newProvider, skipLoadInterface);

                    if (provider != nullptr)
                        *provider = newProvider;
                });

                if (addToList)
                    impl::addProviderName(typeName);
            }

            std::vector<std::string> &getEntries();

        }

        namespace DataFormatter {

            namespace impl {

                using Callback = std::function<std::string(prv::Provider *provider, u64 address, size_t size)>;
                struct Entry {
                    std::string unlocalizedName;
                    Callback callback;
                };

            }

            void add(const std::string &unlocalizedName, const impl::Callback &callback);

            std::vector<impl::Entry> &getEntries();

        }

        namespace FileHandler {

            namespace impl {

                using Callback = std::function<bool(std::filesystem::path)>;
                struct Entry {
                    std::vector<std::string> extensions;
                    Callback callback;
                };

            }

            void add(const std::vector<std::string> &extensions, const impl::Callback &callback);

            std::vector<impl::Entry> &getEntries();

        }

        namespace HexEditor {

            class DataVisualizer {
            public:
                DataVisualizer(u16 bytesPerCell, u16 maxCharsPerCell)
                    : m_bytesPerCell(bytesPerCell), m_maxCharsPerCell(maxCharsPerCell) {}

                virtual ~DataVisualizer() = default;

                virtual void draw(u64 address, const u8 *data, size_t size, bool upperCase) = 0;
                virtual bool drawEditing(u64 address, u8 *data, size_t size, bool upperCase, bool startedEditing) = 0;

                [[nodiscard]] u16 getBytesPerCell() const { return this->m_bytesPerCell; }
                [[nodiscard]] u16 getMaxCharsPerCell() const { return this->m_maxCharsPerCell; }

            protected:
                const static int TextInputFlags;

                bool drawDefaultEditingTextBox(u64 address, const char *format, ImGuiDataType dataType, u8 *data, ImGuiInputTextFlags flags) const;
            private:
                u16 m_bytesPerCell;
                u16 m_maxCharsPerCell;
            };

            namespace impl {

                void addDataVisualizer(const std::string &unlocalizedName, DataVisualizer *visualizer);

                std::map<std::string, DataVisualizer*> &getVisualizers();

            }

            template<std::derived_from<DataVisualizer> T, typename... Args>
            void addDataVisualizer(const std::string &unlocalizedName, Args &&...args) {
                return impl::addDataVisualizer(unlocalizedName, new T(std::forward<Args>(args)...));
            }

        }

        namespace Hashes {

            class Hash {
            public:
                explicit Hash(std::string unlocalizedName) : m_unlocalizedName(std::move(unlocalizedName)) {}

                class Function {
                public:
                    using Callback = std::function<std::vector<u8>(const Region&, prv::Provider *)>;

                    Function(Hash *type, std::string name, Callback callback)
                        : m_type(type), m_name(std::move(name)), m_callback(std::move(callback)) {

                    }

                    [[nodiscard]] Hash *getType() { return this->m_type; }
                    [[nodiscard]] const Hash *getType() const { return this->m_type; }
                    [[nodiscard]] const std::string &getName() const { return this->m_name; }

                    const std::vector<u8>& get(const Region& region, prv::Provider *provider) {
                        if (this->m_cache.empty()) {
                            this->m_cache = this->m_callback(region, provider);
                        }

                        return this->m_cache;
                    }

                    void reset() {
                        this->m_cache.clear();
                    }

                private:
                    Hash *m_type;
                    std::string m_name;
                    Callback m_callback;

                    std::vector<u8> m_cache;
                };

                virtual void draw() { }
                [[nodiscard]] virtual Function create(std::string name) = 0;

                [[nodiscard]] virtual nlohmann::json store() const = 0;
                virtual void load(const nlohmann::json &json) = 0;

                [[nodiscard]] const std::string &getUnlocalizedName() const {
                    return this->m_unlocalizedName;
                }

            protected:
                [[nodiscard]] Function create(const std::string &name, const Function::Callback &callback) {
                    return { this, name, callback };
                }

            private:
                std::string m_unlocalizedName;
            };

            namespace impl {

                std::vector<Hash*> &getHashes();

                void add(Hash* hash);
            }

            template<typename T, typename ... Args>
            void add(Args && ... args) {
                impl::add(new T(std::forward<Args>(args)...));
            }

        }
    }

}
