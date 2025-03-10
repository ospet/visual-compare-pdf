#include "pdf_compare.h"
#include <iostream>
#include <iomanip>

void print_comparison_results(const pdfcompare::ComparisonResult& result) {
    // Print header
    std::cout << "\n=== PDF Comparison Results ===\n" << std::endl;
    
    // Print main results with formatting
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Files are identical: " << (result.identical ? "Yes" : "No") << std::endl;
    std::cout << "Similarity score:    " << (result.similarity * 100) << "%" << std::endl;
    std::cout << "Differing pages:     " << result.differing_pages << std::endl;
    
    // Print list of pages with differences
    if (!result.pages_with_differences.empty()) {
        std::cout << "\nDifferences found on pages: ";
        for (size_t i = 0; i < result.pages_with_differences.size(); ++i) {
            if (i > 0) std::cout << ", ";
            // Add 1 to convert from 0-based to 1-based page numbers
            std::cout << (result.pages_with_differences[i] + 1);
        }
        std::cout << std::endl;
        
        std::cout << "\nDifference images have been generated in the 'diff_output' directory" << std::endl;
    }
    
    std::cout << "\n===========================" << std::endl;
}

int main(int argc, char* argv[]) {
    // take file names from command line
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <pdf1_path> <pdf2_path> <output_dir>" << std::endl;
        return 1;
    }
    const std::string pdf1_path = argv[1];
    const std::string pdf2_path = argv[2];
    const std::string output_dir = argv[3];
    
    try {
        std::cout << "Comparing PDFs:" << std::endl;
        std::cout << "File 1: " << pdf1_path << std::endl;
        std::cout << "File 2: " << pdf2_path << std::endl;
        
        // Perform comparison with 99.9% threshold
        auto result = pdfcompare::compare_pdfs(
            pdf1_path,
            pdf2_path,
            output_dir,
            0.999  // High threshold for strict comparison
        );
        
        // Print results
        print_comparison_results(result);
        
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 