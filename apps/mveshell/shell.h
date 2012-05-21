#ifndef MVEAPP_SHELL_HEADER
#define MVEAPP_SHELL_HEADER

#include "mve/scene.h"

class Shell
{
private:
    mve::Scene::Ptr scene;

private:
    void process_line (std::string const& line);
    void print_help (void);
    void delete_embeddings (std::string const& name);
    void export_embeddings (std::string const& name, std::string const& path);
    void list_embeddings (void);
    void add_exif (std::string const& path);
    std::string readline (void);

public:
    Shell (void);
};

#endif // MVEAPP_SHELL_HEADER
