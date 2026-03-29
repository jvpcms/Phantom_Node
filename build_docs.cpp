#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// String helpers
// ---------------------------------------------------------------------------

static std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t a = s.find_first_not_of(ws);
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);
}

/**
 * Strips comment markers from a raw source line:
 *   "/**"  prefix, leading "*", and trailing "*" / * suffix.
 */
static std::string strip_comment(const std::string& raw) {
    std::string t = trim(raw);
    if (t.size() >= 3 && t.substr(0, 3) == "/**") t = trim(t.substr(3));
    auto end_pos = t.rfind("*/");
    if (end_pos != std::string::npos) t = trim(t.substr(0, end_pos));
    if (!t.empty() && t[0] == '*') t = trim(t.substr(1));
    return t;
}

// ---------------------------------------------------------------------------
// Doc entry
// ---------------------------------------------------------------------------

struct DocEntry {
    std::string docstring;
    std::string signature;
};

// ---------------------------------------------------------------------------
// Parse a single .hpp file for docstring + declaration pairs
// ---------------------------------------------------------------------------

static std::vector<DocEntry> parse_hpp(const fs::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};

    std::vector<std::string> lines;
    for (std::string line; std::getline(file, line);) lines.push_back(line);

    std::vector<DocEntry> entries;

    for (size_t i = 0; i < lines.size();) {
        if (trim(lines[i]).find("/**") == std::string::npos) { ++i; continue; }

        // Collect docstring lines until closing */
        std::string doc;
        size_t j = i;
        while (j < lines.size()) {
            bool closing = trim(lines[j]).find("*/") != std::string::npos;
            std::string cl = strip_comment(lines[j]);
            if (!cl.empty()) {
                if (!doc.empty()) doc += '\n';
                doc += cl;
            }
            ++j;
            if (closing) break;
        }

        // Skip blank lines between comment and declaration
        while (j < lines.size() && trim(lines[j]).empty()) ++j;

        // Collect the declaration (may span lines) until { or ;
        std::string sig;
        for (size_t k = j; k < lines.size() && k < j + 6; ++k) {
            std::string sl = trim(lines[k]);
            if (!sig.empty()) sig += ' ';
            sig += sl;
            if (sl.find('{') != std::string::npos || sl.find(';') != std::string::npos) break;
        }

        // Strip body — keep only up to the opening { or ;
        for (char stop : {'{', ';'}) {
            auto pos = sig.find(stop);
            if (pos != std::string::npos) sig = trim(sig.substr(0, pos));
        }

        // Only record classes, structs, and functions (have parentheses)
        bool is_class = sig.find("class ")  != std::string::npos
                     || sig.find("struct ") != std::string::npos;
        bool is_func  = sig.find('(') != std::string::npos;

        if (!sig.empty() && (is_class || is_func))
            entries.push_back({doc, sig});

        i = j;
    }

    return entries;
}

// ---------------------------------------------------------------------------
// Recursive directory tree renderer
// ---------------------------------------------------------------------------

static std::string render_tree(const fs::path& dir, const std::string& prefix = "") {
    std::vector<fs::path> children;
    for (auto& e : fs::directory_iterator(dir)) children.push_back(e.path());
    std::sort(children.begin(), children.end());

    std::string out;
    for (size_t i = 0; i < children.size(); ++i) {
        bool last = (i == children.size() - 1);
        out += prefix + (last ? "└── " : "├── ") + children[i].filename().string() + '\n';
        if (fs::is_directory(children[i]))
            out += render_tree(children[i], prefix + (last ? "    " : "│   "));
    }
    return out;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    const fs::path src_root = "src";
    const fs::path output   = "docs.md";

    if (!fs::exists(src_root)) {
        std::cerr << "error: src/ not found — run from the project root\n";
        return 1;
    }

    std::ofstream out(output);
    out << "# Phantom_Node — API Reference\n\n";

    // Folder structure
    out << "## Structure\n\n```\nsrc/\n" << render_tree(src_root) << "```\n\n";

    // Collect top-level modules (subdirectories of src/)
    std::vector<fs::path> modules;
    for (auto& e : fs::directory_iterator(src_root))
        if (fs::is_directory(e)) modules.push_back(e.path());
    std::sort(modules.begin(), modules.end(), [](const fs::path& a, const fs::path& b) {
        if (a.filename() == "main") return true;
        if (b.filename() == "main") return false;
        return a < b;
    });

    for (auto& mod : modules) {
        out << "---\n\n## " << mod.filename().string() << "\n\n";

        // Embed docs.md if present
        fs::path module_docs = mod / "docs.md";
        if (fs::exists(module_docs)) {
            std::ifstream docs_file(module_docs);
            std::string line;
            // Skip the first heading line — we already printed the module name above
            std::getline(docs_file, line);
            while (std::getline(docs_file, line)) out << line << '\n';
            out << "\n";
        }

        // Collect .hpp files in this module, sorted
        std::vector<fs::path> headers;
        for (auto& e : fs::recursive_directory_iterator(mod))
            if (e.path().extension() == ".hpp") headers.push_back(e.path());
        std::sort(headers.begin(), headers.end());

        for (auto& header : headers) {
            out << "### `" << fs::relative(header, src_root).string() << "`\n\n";

            for (auto& entry : parse_hpp(header)) {
                out << "#### `" << entry.signature << "`\n\n";
                if (!entry.docstring.empty())
                    out << entry.docstring << "\n\n";
            }
        }
    }

    std::cout << "wrote " << output.string() << '\n';
}
