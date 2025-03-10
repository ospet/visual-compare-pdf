#include "pdf_compare.h"
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-image.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <png.h>
#include <memory>
#include <vector>
#include <cmath>
#include <filesystem>
#include <fstream> 
#include <sstream>

namespace pdfcompare {

namespace {
    struct ImageDiff {
        double similarity;
        std::vector<unsigned char> diff_image;
    };

    // Helper function to compare two RGBA buffers and generate difference image
    ImageDiff compare_buffers(const char* buf1, 
                            const char* buf2, 
                            size_t width, 
                            size_t height) {
        size_t pixels = width * height;
        size_t total_components = pixels * 4; // RGBA = 4 channels
        size_t differences = 0;
        
        // Create difference image (RGBA format)
        std::vector<unsigned char> diff_image(total_components, 0);
        
        for (size_t i = 0; i < pixels; ++i) {
            size_t pixel_offset = i * 4;
            bool pixel_different = false;
            
            // Compare RGB components
            for (size_t c = 0; c < 3; ++c) {
                if (buf1[pixel_offset + c] != buf2[pixel_offset + c]) {
                    pixel_different = true;
                    differences++;
                    break;
                }
            }
            
            // Compare alpha channel
            if (buf1[pixel_offset + 3] != buf2[pixel_offset + 3]) {
                pixel_different = true;
                differences++;
            }
            
            // If pixel is different, mark it red in diff image
            if (pixel_different) {
                diff_image[pixel_offset + 0] = 255;  // R
                diff_image[pixel_offset + 1] = 0;    // G
                diff_image[pixel_offset + 2] = 0;    // B
                diff_image[pixel_offset + 3] = 255;  // A
            } else {
                // Copy original pixel (with slight transparency)
                diff_image[pixel_offset + 0] = buf1[pixel_offset + 0];
                diff_image[pixel_offset + 1] = buf1[pixel_offset + 1];
                diff_image[pixel_offset + 2] = buf1[pixel_offset + 2];
                diff_image[pixel_offset + 3] = 128;  // Semi-transparent
            }
        }
        
        double similarity = 1.0 - (static_cast<double>(differences) / pixels);
        return {similarity, diff_image};
    }

    // Helper function to save RGBA buffer as PNG
    void save_png(const std::string& filename, 
                 const unsigned char* buffer,
                 size_t width, 
                 size_t height) {
        FILE* fp = fopen(filename.c_str(), "wb");
        if (!fp) {
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }

        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) {
            fclose(fp);
            throw std::runtime_error("Failed to create PNG write struct");
        }

        png_infop info = png_create_info_struct(png);
        if (!info) {
            png_destroy_write_struct(&png, nullptr);
            fclose(fp);
            throw std::runtime_error("Failed to create PNG info struct");
        }

        if (setjmp(png_jmpbuf(png))) {
            png_destroy_write_struct(&png, &info);
            fclose(fp);
            throw std::runtime_error("Error during PNG creation");
        }

        png_init_io(png, fp);
        png_set_IHDR(png, info, width, height, 8,
                     PNG_COLOR_TYPE_RGBA,
                     PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT,
                     PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png, info);

        std::vector<png_bytep> row_pointers(height);
        for (size_t y = 0; y < height; y++) {
            row_pointers[y] = (png_bytep)&buffer[y * width * 4];
        }

        png_write_image(png, row_pointers.data());
        png_write_end(png, nullptr);

        png_destroy_write_struct(&png, &info);
        fclose(fp);
    }
}

ComparisonResult compare_pdfs(const std::string& pdf1_path, 
                            const std::string& pdf2_path,
                            const std::string& output_dir,
                            double threshold) {
    ComparisonResult result = {true, 1.0, 0};
    result.pages_with_differences.clear();
    
    // Create output directory if it doesn't exist
    std::filesystem::create_directories(output_dir);
    
    // Load PDFs
    std::unique_ptr<poppler::document> doc1(poppler::document::load_from_file(pdf1_path));
    std::unique_ptr<poppler::document> doc2(poppler::document::load_from_file(pdf2_path));
    
    if (!doc1 || !doc2) {
        throw std::runtime_error("Failed to load PDF files");
    }
    
    if (doc1->pages() != doc2->pages()) {
        result.identical = false;
        result.similarity = 0.0;
        result.differing_pages = abs(doc1->pages() - doc2->pages());
        return result;
    }
    
    double total_similarity = 0.0;
    int different_pages = 0;
    
    // Compare each page
    for (int i = 0; i < doc1->pages(); ++i) {
        std::unique_ptr<poppler::page> page1(doc1->create_page(i));
        std::unique_ptr<poppler::page> page2(doc2->create_page(i));
        
        if (!page1 || !page2) {
            throw std::runtime_error("Failed to load page " + std::to_string(i));
        }

        // Get page dimensions
        poppler::rectf rect1 = page1->page_rect();
        poppler::rectf rect2 = page2->page_rect();

        // Convert pages to images (using page_renderer)
        poppler::page_renderer renderer;
        renderer.set_render_hints(
            poppler::page_renderer::antialiasing | 
            poppler::page_renderer::text_antialiasing
        );

        poppler::image img1(renderer.render_page(page1.get(), 72.0, 72.0));
        poppler::image img2(renderer.render_page(page2.get(), 72.0, 72.0));
        
        // Ensure images are the same size
        if (img1.width() != img2.width() || img1.height() != img2.height()) {
            result.identical = false;
            result.differing_pages++;
            result.pages_with_differences.push_back(i);
            continue;
        }
        
        // Compare images and get difference image
        auto diff_result = compare_buffers(
            img1.data(),
            img2.data(),
            img1.width(),
            img1.height()
        );
        
        if (diff_result.similarity < threshold) {
            result.identical = false;
            different_pages++;
            result.pages_with_differences.push_back(i);
            
            // Save difference image
            std::ostringstream filename;
            filename << output_dir << "/diff_page_" << (i + 1) << ".png";
            save_png(filename.str(), 
                    diff_result.diff_image.data(),
                    img1.width(), 
                    img1.height());
        }
        
        total_similarity += diff_result.similarity;
    }
    
    result.similarity = total_similarity / doc1->pages();
    result.differing_pages = different_pages;
    
    return result;
}

}  // namespace pdfcompare 