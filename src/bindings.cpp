#include <emscripten/bind.h>
#include <emscripten/val.h>
//#include <emscripten/typed_memory_view.h>
#include <vector>
#include <string>

using namespace emscripten;

class PDFCompareWasm {
public:
    val compareImages(const val& imageData1, 
                     const val& imageData2,
                     int width,
                     int height) {
        // Get the length of the image data (RGBA = 4 bytes per pixel)
        const size_t length = width * height * 4;
        
        // Create vectors to hold the image data
        std::vector<uint8_t> data1(length);
        std::vector<uint8_t> data2(length);
        
        // Get typed arrays from JavaScript
        val uint8Array1 = val::global("Uint8Array").new_(imageData1);
        val uint8Array2 = val::global("Uint8Array").new_(imageData2);
        
        // Copy data from JavaScript TypedArrays to C++ vectors
        for (size_t i = 0; i < length; ++i) {
            data1[i] = uint8Array1[i].as<uint8_t>();
            data2[i] = uint8Array2[i].as<uint8_t>();
        }
        
        // Create the difference image
        std::vector<uint8_t> diffImage(length);
        size_t differences = 0;
        
        // Compare images pixel by pixel
        for (size_t i = 0; i < length; i += 4) {
            bool pixelDifferent = false;
            for (size_t j = 0; j < 4; ++j) {
                if (data1[i + j] != data2[i + j]) {
                    pixelDifferent = true;
                    break;
                }
            }
            
            if (pixelDifferent) {
                // Mark difference in red
                diffImage[i] = 255;     // R
                diffImage[i + 1] = 0;   // G
                diffImage[i + 2] = 0;   // B
                diffImage[i + 3] = 255; // A
                differences++;
            } else {
                // Copy original pixel with reduced alpha
                diffImage[i] = data1[i];        // R
                diffImage[i + 1] = data1[i + 1];// G
                diffImage[i + 2] = data1[i + 2];// B
                diffImage[i + 3] = 255;         // A (semi-transparent)
            }
        }
        
        // Calculate similarity
        double similarity = 1.0 - (static_cast<double>(differences) / (width * height));
        bool identical = (differences == 0);
        
        // Create return object
        val result = val::object();
        result.set("identical", identical);
        result.set("similarity", similarity);
        result.set("differingPixels", differences);
        
        // Create a new Uint8Array for the diff image
        val diffArray = val::global("Uint8Array").new_(typed_memory_view(length, diffImage.data()));
        result.set("diffImage", diffArray);
        
        return result;
    }
};

EMSCRIPTEN_BINDINGS(pdf_compare_module) {
    class_<PDFCompareWasm>("PDFCompare")
        .constructor<>()
        .function("compareImages", &PDFCompareWasm::compareImages);
} 