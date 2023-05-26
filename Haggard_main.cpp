#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <string>
using namespace std;

//***************************************************************************************************//
//                      THIS SECTION WAS GIVEN BY THE PROFESSOR                                  //
//***************************************************************************************************//

// Pixel structure
struct Pixel
{
    // Red, green, blue color values
    int red;
    int green;
    int blue;
};

/**
 * Gets an integer from a binary stream.
 * Helper function for read_image()
 * @param stream the stream
 * @param offset the offset at which to read the integer
 * @param bytes  the number of bytes to read
 * @return the integer starting at the given offset
 */ 
int get_int(fstream& stream, int offset, int bytes)
{
    stream.seekg(offset);
    int result = 0;
    int base = 1;
    for (int i = 0; i < bytes; i++)
    {   
        result = result + stream.get() * base;
        base = base * 256;
    }
    return result;
}

/**
 * Reads the BMP image specified and returns the resulting image as a vector
 * @param filename BMP image filename
 * @return the image as a vector of vector of Pixels
 */
vector<vector<Pixel>> read_image(string filename)
{
    // Open the binary file
    fstream stream;
    stream.open(filename, ios::in | ios::binary);

    // Get the image properties
    int file_size = get_int(stream, 2, 4);
    int start = get_int(stream, 10, 4);
    int width = get_int(stream, 18, 4);
    int height = get_int(stream, 22, 4);
    int bits_per_pixel = get_int(stream, 28, 2);

    // Scan lines must occupy multiples of four bytes
    int scanline_size = width * (bits_per_pixel / 8);
    int padding = 0;
    if (scanline_size % 4 != 0)
    {
        padding = 4 - scanline_size % 4;
    }

    // Return empty vector if this is not a valid image
    if (file_size != start + (scanline_size + padding) * height)
    {
        return {};
    }

    // Create a vector the size of the input image
    vector<vector<Pixel>> image(height, vector<Pixel> (width));

    int pos = start;
    // For each row, starting from the last row to the first
    // Note: BMP files store pixels from bottom to top
    for (int i = height - 1; i >= 0; i--)
    {
        // For each column
        for (int j = 0; j < width; j++)
        {
            // Go to the pixel position
            stream.seekg(pos);

            // Save the pixel values to the image vector
            // Note: BMP files store pixels in blue, green, red order
            image[i][j].blue = stream.get();
            image[i][j].green = stream.get();
            image[i][j].red = stream.get();

            // We are ignoring the alpha channel if there is one

            // Advance the position to the next pixel
            pos = pos + (bits_per_pixel / 8);
        }

        // Skip the padding at the end of each row
        stream.seekg(padding, ios::cur);
        pos = pos + padding;
    }

    // Close the stream and return the image vector
    stream.close();
    return image;
}

/**
 * Sets a value to the char array starting at the offset using the size
 * specified by the bytes.
 * This is a helper function for write_image()
 * @param arr    Array to set values for
 * @param offset Starting index offset
 * @param bytes  Number of bytes to set
 * @param value  Value to set
 * @return nothing
 */
void set_bytes(unsigned char arr[], int offset, int bytes, int value)
{
    for (int i = 0; i < bytes; i++)
    {
        arr[offset+i] = (unsigned char)(value>>(i*8));
    }
}

/**
 * Write the input image to a BMP file name specified
 * @param filename The BMP file name to save the image to
 * @param image    The input image to save
 * @return True if successful and false otherwise
 */
bool write_image(string filename, const vector<vector<Pixel>>& image)
{
    // Get the image width and height in pixels
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Calculate the width in bytes incorporating padding (4 byte alignment)
    int width_bytes = width_pixels * 3;
    int padding_bytes = 0;
    padding_bytes = (4 - width_bytes % 4) % 4;
    width_bytes = width_bytes + padding_bytes;

    // Pixel array size in bytes, including padding
    int array_bytes = width_bytes * height_pixels;

    // Open a file stream for writing to a binary file
    fstream stream;
    stream.open(filename, ios::out | ios::binary);

    // If there was a problem opening the file, return false
    if (!stream.is_open())
    {
        return false;
    }

    // Create the BMP and DIB Headers
    const int BMP_HEADER_SIZE = 14;
    const int DIB_HEADER_SIZE = 40;
    unsigned char bmp_header[BMP_HEADER_SIZE] = {0};
    unsigned char dib_header[DIB_HEADER_SIZE] = {0};

    // BMP Header
    set_bytes(bmp_header,  0, 1, 'B');              // ID field
    set_bytes(bmp_header,  1, 1, 'M');              // ID field
    set_bytes(bmp_header,  2, 4, BMP_HEADER_SIZE+DIB_HEADER_SIZE+array_bytes); // Size of BMP file
    set_bytes(bmp_header,  6, 2, 0);                // Reserved
    set_bytes(bmp_header,  8, 2, 0);                // Reserved
    set_bytes(bmp_header, 10, 4, BMP_HEADER_SIZE+DIB_HEADER_SIZE); // Pixel array offset

    // DIB Header
    set_bytes(dib_header,  0, 4, DIB_HEADER_SIZE);  // DIB header size
    set_bytes(dib_header,  4, 4, width_pixels);     // Width of bitmap in pixels
    set_bytes(dib_header,  8, 4, height_pixels);    // Height of bitmap in pixels
    set_bytes(dib_header, 12, 2, 1);                // Number of color planes
    set_bytes(dib_header, 14, 2, 24);               // Number of bits per pixel
    set_bytes(dib_header, 16, 4, 0);                // Compression method (0=BI_RGB)
    set_bytes(dib_header, 20, 4, array_bytes);      // Size of raw bitmap data (including padding)                     
    set_bytes(dib_header, 24, 4, 2835);             // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 28, 4, 2835);             // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 32, 4, 0);                // Number of colors in palette
    set_bytes(dib_header, 36, 4, 0);                // Number of important colors

    // Write the BMP and DIB Headers to the file
    stream.write((char*)bmp_header, sizeof(bmp_header));
    stream.write((char*)dib_header, sizeof(dib_header));

    // Initialize pixel and padding
    unsigned char pixel[3] = {0};
    unsigned char padding[3] = {0};

    // Pixel Array (Left to right, bottom to top, with padding)
    for (int h = height_pixels - 1; h >= 0; h--)
    {
        for (int w = 0; w < width_pixels; w++)
        {
            // Write the pixel (Blue, Green, Red)
            pixel[0] = image[h][w].blue;
            pixel[1] = image[h][w].green;
            pixel[2] = image[h][w].red;
            stream.write((char*)pixel, 3);
        }
        // Write the padding bytes
        stream.write((char *)padding, padding_bytes);
    }

    // Close the stream and return true
    stream.close();
    return true;
}

//***************************************************************************************************//
//                                THIS SECTION WAS GIVEN BY THE PROFESSOR                                    //
//***************************************************************************************************//

// Process 1 (Vignette)

vector<vector<Pixel>> process_1(const vector<vector<Pixel>>& image)
{
    // Set variables

    double num_columns = image[0].size(); // WIDTH
    double num_rows = image.size();       // HEIGHT

    // Define new 2D vector

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));

    // Iterate through row and col

    for (int row = 0; row < num_rows; row++)
        {
            for (int col = 0; col < num_columns; col++)
            {
                // Read in image data

                int red_color = image[row][col].red;
                int green_color = image[row][col].green;
                int blue_color = image[row][col].blue;
                
				// Perform the operation on the color values

                double distance = sqrt(pow((col - num_columns/2), 2) + pow((row - num_rows/2), 2));
                double scaling_factor = (num_rows - distance)/num_rows;
                //cout << scaling_factor << endl;
                int newred = red_color * scaling_factor;
                int newgreen = green_color * scaling_factor;
                int newblue = blue_color * scaling_factor;

                // write new image color values

                new_image[row][col].red = newred;
                new_image[row][col].green = newgreen;
                new_image[row][col].blue = newblue;
                
            }
        }
    // return new image

    return new_image;

}

// Process 2 (Clarendon - darks darker and lights lighter)

vector<vector<Pixel>> process_2(const vector<vector<Pixel>>& image, double scaling_factor)
{
    // Set variables

    double num_rows = image.size();       // HEIGHT
    double num_columns = image[0].size(); // WIDTH
    
    // Define new empty 2D vector

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));

    // Iterate through row and col

    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            // Read in image data

            int red_value = image[row][col].red;
            int green_value = image[row][col].green;
            int blue_value = image[row][col].blue;
            
            // Perform the operation on the color values

            double avg_value = ((red_value + green_value + blue_value) / 3);
           
            if (avg_value >= 170) // lights lighter
            {
                new_image[row][col].red = (255 - (255 - red_value) * scaling_factor);                    
                new_image[row][col].green = (255 - (255 - green_value) * scaling_factor);
                new_image[row][col].blue = (255 - (255 - blue_value) * scaling_factor);
            }
            else if (avg_value < 90) // darks darker
            {
                new_image[row][col].red = red_value * scaling_factor;
                new_image[row][col].green = green_value * scaling_factor;
                new_image[row][col].blue = blue_value * scaling_factor;
            }
            else // stays the same
            {
                new_image[row][col].red = red_value;
                new_image[row][col].green = green_value;
                new_image[row][col].blue = blue_value; 
            }

            // write new image color values

            // new_image[row][col].red = red_value;
            // new_image[row][col].green = green_value;
            // new_image[row][col].blue = blue_value;

        }
    }
    // return new image
    
    return new_image;
}

// Process 3 (Greyscale)

vector<vector<Pixel>> process_3(const vector<vector<Pixel>>& image)
{
    // Set variables

    double num_rows = image.size();       // HEIGHT
    double num_columns = image[0].size(); // WIDTH
    
    // Define new empty 2D vector

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));

    // Iterate through row and col

    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            // Read in image data

            int red_value = image[row][col].red;
            int green_value = image[row][col].green;
            int blue_value = image[row][col].blue;
            
            // Perform the operation on the color values

            int grey_value = (red_value + green_value + blue_value) / 3;
           
            int newred = grey_value;
            int newgreen = grey_value;
            int newblue = grey_value;

            // write new image color values

            new_image[row][col].red = newred;
            new_image[row][col].green = newgreen;
            new_image[row][col].blue = newblue; 

        }
    }
    // return new image
    
    return new_image;
}

// Process 4 (Rotate by 90 clockwise)

vector<vector<Pixel>> process_4(const vector<vector<Pixel>>& image)
{
    // Set variables

    double num_rows = image.size();       // HEIGHT
    double num_columns = image[0].size(); // WIDTH
    
    // Define new empty 2D vector

    vector<vector<Pixel>> new_image(num_columns, vector<Pixel> (num_rows));

    // Iterate through row and col

    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            new_image[col][num_rows - row - 1] = image[row][col];
        }
    }
    // return new image
    
    return new_image;
}

// Process 5 (Rotate by multiples of 90 clockwise)


vector<vector<Pixel>> process_5(const vector<vector<Pixel>>& image, int number)
{
    int angle = number * 90;

    if (angle % 90 != 0)
    {
        cout << "angle must be a multiple of 90 degrees" << endl;        
    }
    else if (angle % 360 == 0)
    {
        return image;
    }
    else if (angle % 360 == 90)
    {
        return process_4(image);
    }
    else if (angle % 360 == 180)
    {
        return process_4(process_4(image));
    }
    else
    {
        return process_4(process_4(process_4(image)));
    }
}

// Process 6 (Scale image x and y direction)

vector<vector<Pixel>> process_6(const vector<vector<Pixel>>& image, int x_scale, int y_scale)
{
    // Set variables

    double num_rows = image.size();       // HEIGHT
    double num_columns = image[0].size(); // WIDTH
    
    // Define new empty 2D vector

    vector<vector<Pixel>> new_image(num_rows * y_scale, vector<Pixel> (num_columns * x_scale));

    // Iterate through row and col

    for (int row = 0; row < y_scale * num_rows; row++)
    {
        for (int col = 0; col < x_scale * num_columns; col++)
        {
            new_image[row][col] = image[row / y_scale][col / x_scale];
        }
    }
    // return new image
    
    return new_image;
}

// Process 7 High Contrast

vector<vector<Pixel>> process_7(const vector<vector<Pixel>>& image)
{
    // Set variables

    double num_rows = image.size();       // HEIGHT
    double num_columns = image[0].size(); // WIDTH
    
    // Define new empty 2D vector

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));

    // Iterate through row and col

    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            // Read in image data

            int red_value = image[row][col].red;
            int green_value = image[row][col].green;
            int blue_value = image[row][col].blue;
            int newred, newgreen, newblue;

            // Perform the operation on the color values

            int grey_value = (red_value + green_value + blue_value) / 3;
           
            if (grey_value >= 255 / 2)
            {
                newred = 255;
                newgreen = 255;
                newblue = 255;
            }
            else
            {
                newred = 0;
                newgreen = 0;
                newblue = 0;
            }

            // write new image color values

            new_image[row][col].red = newred;
            new_image[row][col].green = newgreen;
            new_image[row][col].blue = newblue; 
        }
    }
    // return new image
    
    return new_image;
}

// Process 8 Lighten

vector<vector<Pixel>> process_8(const vector<vector<Pixel>>& image, double scaling_factor)
{
    // Set variables

    double num_rows = image.size();       // HEIGHT
    double num_columns = image[0].size(); // WIDTH
    
    // Define new empty 2D vector

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));

    // Iterate through row and col

    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            // Read in image data

            int red_value = image[row][col].red;
            int green_value = image[row][col].green;
            int blue_value = image[row][col].blue;
            
            // Perform the operation on the color values

            int newred = 255 - (255 - red_value) * scaling_factor;
            int newgreen = 255 - (255 - green_value) * scaling_factor;
            int newblue = 255 - (255 - blue_value) * scaling_factor;

            // write new image color values

            new_image[row][col].red = newred;
            new_image[row][col].green = newgreen;
            new_image[row][col].blue = newblue; 
        }
    }
    // return new image
    
    return new_image;
}

// Process 9 Darken

vector<vector<Pixel>> process_9(const vector<vector<Pixel>>& image, double scaling_factor)
{
    // Set variables

    double num_rows = image.size();       // HEIGHT
    double num_columns = image[0].size(); // WIDTH
    
    // Define new empty 2D vector

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));

    // Iterate through row and col

    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            // Read in image data

            int red_value = image[row][col].red;
            int green_value = image[row][col].green;
            int blue_value = image[row][col].blue;
            
            // Perform the operation on the color values

            int newred = red_value * scaling_factor;
            int newgreen = green_value * scaling_factor;
            int newblue = blue_value * scaling_factor;

            // write new image color values

            new_image[row][col].red = newred;
            new_image[row][col].green = newgreen;
            new_image[row][col].blue = newblue; 
        }
    }
    // return new image
    
    return new_image;
}

vector<vector<Pixel>> process_10(const vector<vector<Pixel>>& image)
{
    // Set variables

    double num_rows = image.size();       // HEIGHT
    double num_columns = image[0].size(); // WIDTH
    
    // Define new empty 2D vector

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns));

    // Iterate through row and col

    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            // Read in image data

            int red_value = image[row][col].red;
            int green_value = image[row][col].green;
            int blue_value = image[row][col].blue;

            // Perform the operation on the color values

            int max_color = max(red_value, blue_value);
            int max_color1 = max(max_color, green_value);
            
            // Set new color values
            if (red_value + green_value + blue_value >= 550)
            {
                new_image[row][col].red = 255;
                new_image[row][col].green = 255;
                new_image[row][col].blue = 255;
            }
            else if (red_value + green_value + blue_value <= 150)
            {
                new_image[row][col].red = 0;
                new_image[row][col].green = 0;
                new_image[row][col].blue = 0;
            }
            else if (max_color1 == red_value)
            {
                new_image[row][col].red = 255;
                new_image[row][col].green = 0;
                new_image[row][col].blue = 0;
            }
            else if (max_color1 == green_value)
            {
                new_image[row][col].red = 0;
                new_image[row][col].green = 255;
                new_image[row][col].blue = 0;
            }
            else if (max_color1 == blue_value)
            {
                new_image[row][col].red = 0;
                new_image[row][col].green = 0;
                new_image[row][col].blue = 255;
            }

            // write new image color values

            // new_image[row][col].red = newred;
            // new_image[row][col].green = newgreen;
            // new_image[row][col].blue = newblue; 
        }
    }
    // return new image
    
    return new_image;
}

int main()
{
    cout << "CSPB 1300 Image Processing Application\n";
    cout << "Enter the name of the BMP file to process: ";
    string filename;

    while (cin >> filename)
    {
        menu:
        cout << "---------------------------------------\n\n";
        cout << "IMAGE PROCESSING MENU\n\n";
        cout << "0) Change image (current: " << filename << ")\n";
        cout << "1) Vignette\n";
        cout << "2) Clarendon\n";
        cout << "3) Grayscale\n";
        cout << "4) Rotate by 90 degrees clockwise\n";
        cout << "5) Rotate by multiple 90 degrees\n";
        cout << "6) Enlarge in x and y direction\n";
        cout << "7) High contrast black and white\n";
        cout << "8) Lighten by scaling factor\n";
        cout << "9) Darken by scaling factor\n";
        cout << "10) Convert to black, white, red, blue, green only\n\n";
        cout << "Enter your selection (Q to quit): ";
        string user_input;
        cin >> user_input;
        cout << "\n";   

        if (user_input == "0")
        {
            cout << "Change image selected\n\n";
            cout << "Enter the name of the new BMP file to process: ";
            cin >> filename;
            cout << "\n";
            cout << "New Filename: " << filename << "\n\n";
            vector<vector<Pixel>> image = read_image(filename);
            cout << "Successfully changed image to " << filename << "!" << "\n";
            goto menu;
        }
        else if (user_input == "1")
        {
            vector<vector<Pixel>> image = read_image(filename);
            cout << "Vignette selected\n\n";
            cout << "Enter output BMP filename: ";
            string new_filename;
            cin >> new_filename;
            cout << "\n";
            cout << "New Filename: " << new_filename << "\n\n";
            vector<vector<Pixel>> new_image = process_1(image);
            bool success = write_image(new_filename, new_image);
            cout << "Successfully applied vignette!\n\n\n";
            goto menu;
        }
        else if (user_input == "2")
        {
            vector<vector<Pixel>> image = read_image(filename);
            cout << "Clarendon selected\n\n";
            cout << "Enter scaling factor: ";
            double scaling_factor;
            cin >> scaling_factor;
            cout << "\n";
            cout << "Selected scaling factor: " << scaling_factor << "\n\n";
            cout << "Enter output BMP filename: ";
            string new_filename;
            cin >> new_filename;
            cout << "\n";
            cout << "New Filename: " << new_filename << "\n\n";
            vector<vector<Pixel>> new_image = process_2(image, scaling_factor);
            bool success = write_image(new_filename, new_image);
            cout << "Successfully applied clarendon!" << "\n";
            goto menu;            
        }
        else if (user_input == "3")
        {
            vector<vector<Pixel>> image = read_image(filename);
            cout << "Grayscale selected\n\n";
            cout << "Enter output BMP filename: ";
            string new_filename;
            cin >> new_filename;
            cout << "\n";
            cout << "New Filename: " << new_filename << "\n\n";
            vector<vector<Pixel>> new_image = process_3(image);
            bool success = write_image(new_filename, new_image);
            cout << "Successfully applied grayscale!" << "\n";
            goto menu;
        }
        else if (user_input == "4")
        {
            vector<vector<Pixel>> image = read_image(filename);
            cout << "Rotate 90 degrees selected\n\n";
            cout << "Enter output BMP filename: ";
            string new_filename;
            cin >> new_filename;
            cout << "\n";
            cout << "New Filename: " << new_filename << "\n\n";
            vector<vector<Pixel>> new_image = process_4(image);
            bool success = write_image(new_filename, new_image);
            cout << "Successfully applied 90 degree rotation!" << "\n";
            goto menu;
        }
        else if (user_input == "5")
        {
            vector<vector<Pixel>> image = read_image(filename);
            cout << "Rotate multiple 90 degrees selected\n\n";
            cout << "Enter number of 90 degree rotations: ";
            double rotations;
            cin >> rotations;
            cout << "\n";
            cout << "Selected number of rotations: " << rotations << "\n\n";
            cout << "Enter output BMP filename: ";
            string new_filename;
            cin >> new_filename;
            cout << "\n";
            cout << "New Filename: " << new_filename << "\n\n";
            vector<vector<Pixel>> new_image = process_5(image, rotations);
            bool success = write_image(new_filename, new_image);
            cout << "Successfully applied multiple 90 degree rotations!" << "\n";
            goto menu;
        }
        else if (user_input == "6")
        {
            vector<vector<Pixel>> image = read_image(filename);
            cout << "Scale image selected\n\n";
            cout << "Enter X scale integer > 1: ";
            double x_scale;
            cin >> x_scale;
            cout << "\n";
            cout << "Selected X scale: " << x_scale << "\n\n";
            cout << "Enter Y scale > 1: ";
            double y_scale;
            cin >> y_scale;
            cout << "\n";
            cout << "Selected Y scale: " << y_scale << "\n\n";
            cout << "Enter output BMP filename: ";
            string new_filename;
            cin >> new_filename;
            cout << "\n";
            cout << "New Filename: " << new_filename << "\n\n";
            vector<vector<Pixel>> new_image = process_6(image, x_scale, y_scale);
            bool success = write_image(new_filename, new_image);
            cout << "Successfully scaled!" << "\n";
            goto menu;
        }
        else if (user_input == "7")
        {
            vector<vector<Pixel>> image = read_image(filename);
            cout << "High contrast selected\n\n";
            cout << "Enter output BMP filename: ";
            string new_filename;
            cin >> new_filename;
            cout << "\n";
            cout << "New Filename: " << new_filename << "\n\n";
            vector<vector<Pixel>> new_image = process_7(image);
            bool success = write_image(new_filename, new_image);
            cout << "Successfully applied high contrast!" << "\n";
            goto menu;
        }
        else if (user_input == "8")
        {
            vector<vector<Pixel>> image = read_image(filename);
            cout << "Lighten selected\n\n";
            cout << "Enter scaling factor: ";
            double scaling_factor;
            cin >> scaling_factor;
            cout << "\n";
            cout << "Selected scaling factor: " << scaling_factor << "\n\n";
            cout << "Enter output BMP filename: ";
            string new_filename;
            cin >> new_filename;
            cout << "\n";
            cout << "New Filename: " << new_filename << "\n\n";
            vector<vector<Pixel>> new_image = process_8(image, scaling_factor);               
            bool success = write_image(new_filename, new_image);
            cout << "Successfully lightened!" << "\n";
            goto menu;
        }
        else if (user_input == "9")
        {
            vector<vector<Pixel>> image = read_image(filename);
            cout << "Darken selected\n\n";
            cout << "Enter scaling factor: ";
            double scaling_factor;
            cin >> scaling_factor;
            cout << "\n";
            cout << "Selected scaling factor: " << scaling_factor << "\n\n";
            cout << "Enter output BMP filename: ";
            string new_filename;
            cin >> new_filename;
            cout << "\n";
            cout << "New Filename: " << new_filename << "\n\n";
            vector<vector<Pixel>> new_image = process_9(image, scaling_factor);
            bool success = write_image(new_filename, new_image);
            cout << "Successfully darkened!" << "\n";
            goto menu;
        }
        else if (user_input == "10")
        {
            vector<vector<Pixel>> image = read_image(filename);
            cout << "Black, white, red, green, blue selected\n\n";
            cout << "Enter output BMP filename: ";
            string new_filename;
            cin >> new_filename;
            cout << "\n";
            cout << "New Filename: " << new_filename << "\n\n";
            vector<vector<Pixel>> new_image = process_10(image);
            bool success = write_image(new_filename, new_image);
            cout << "Successfully applied black, white, red, green, blue!" << "\n";
            goto menu;
        }
        else if (user_input == "Q")
        {
            cout << "Goodbye! Program will now close. Have a great day!\n\n";
            cout << "---------------------------------------\n\n";
            break;
        }    
    }
    return 0;
}