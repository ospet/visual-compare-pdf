#ifndef PDF_COMPARE_H
#define PDF_COMPARE_H

#include <string>
#include <vector>

// #ifdef _WIN32
//     // #ifdef PDF_COMPARE_EXPORTS
//     //     #define PDF_COMPARE_API __declspec(dllexport)
//     // #else
//     //     #define PDF_COMPARE_API __declspec(dllimport)
//     // #endif
// #else
//     #define PDF_COMPARE_API
// #endif

namespace pdfcompare {

struct ComparisonResult {
    bool identical;
    double similarity;
    int differing_pages;
    std::vector<int> pages_with_differences; // Store which pages had differences
};

/*PDF_COMPARE_API*/ ComparisonResult compare_pdfs(const std::string& pdf1_path, 
                                            const std::string& pdf2_path,
                                            const std::string& output_dir,
                                            double threshold = 0.99);

}  // namespace pdfcompare

#endif // PDF_COMPARE_H 