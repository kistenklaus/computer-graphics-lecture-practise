#include <cglib/core/image.h>
#include <cglib/core/stb_image.h>
#include <cglib/core/stb_image_write.h>
#include <cglib/core/assert.h>

#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>

Image::Image() : m_width(0), m_height(0)
{ }

Image::Image(int width, int height) : 
    m_width(width), 
    m_height(height), 
    m_pixels(width* height)
{ }

void Image::setSize(int width, int height)
{
    m_width = width;
    m_height = height;
    m_pixels.resize(m_width * m_height);
}

const glm::vec4& Image::getPixel(int i, int j) const
{
    cg_assert(i >= 0);
    cg_assert(j >= 0);
    cg_assert(i < m_width);
    cg_assert(j < m_height);
    return m_pixels[i + j * m_width];
}
	
const glm::vec4* Image::getPixels() const
{
    return m_pixels.data();
}

glm::vec4* Image::getPixels()
{
    return m_pixels.data();
}

void Image::setPixel(int i, int j, const glm::vec4& pixel)
{
    cg_assert(i >= 0);
    cg_assert(j >= 0);
    cg_assert(i < m_width);
    cg_assert(j < m_height);
    m_pixels[i + j * m_width] = pixel;
}

float const* Image::raw_data() const
{
    return reinterpret_cast<float const*>(m_pixels.data());
}

float* Image::raw_data()
{
    return reinterpret_cast<float*>(m_pixels.data());
}

void Image::clear(glm::vec4 const& color)
{
    for (int i = 0; i < m_width * m_height; i++)
    {
        m_pixels[i] = color;
    }
}

void Image::save(std::string const& path, float gamma) const
{
    size_t lastindex = path.find_last_of("."); 
    const std::string extension = path.substr(lastindex+1, path.length());

    std::vector<std::uint8_t> bgr(m_pixels.size() * 3);
	
    for (int y = 0; y < m_height; ++y)
    for (int x = 0; x < m_width;  ++x)
    {
        std::size_t const idx_dst = 3*((m_height-y-1) * m_width + x);
        std::size_t const idx_src = (y * m_width + x);

        for (int c = 0; c < 3; ++c)
        {
            float const gamma_corrected = std::pow(std::max(0.f, m_pixels[idx_src][c]), 1.f / gamma);
            float const mapped          = std::max<float>(0.0f, std::min<float>(255.0f, 255.f * gamma_corrected));
            bgr[idx_dst+c]              = uint8_t(mapped);
        }
    }
    if (extension == "tga") 
        cg_assert(false && "dont!");
    else if (extension == "png")
        stbi_write_png(path.c_str(), m_width, m_height, 3, bgr.data(), 3*m_width);
    else 
        cg_assert(false && "not_implemented");
}

void Image::load(std::string const& path, float gamma)
{
	int num_components;
	stbi_ldr_to_hdr_gamma(gamma);
    float *data = stbi_loadf(path.c_str(), &m_width, &m_height, &num_components, 4);
	if(!data) {
		std::cerr << "error: could not load image \"" << path << "\"" << std::endl;
    m_width = m_height = 1;
    m_pixels.resize(1);
		return;
	}
	m_pixels.resize(m_width * m_height);
	/* flip image in Y */
	for(int y = 0; y < m_height; y++) {
		memcpy(&m_pixels[(m_height - y - 1) * m_width],
				data + y * m_width * 4,
				4 * m_width * sizeof(float));
	}
	stbi_image_free(data);
}

void Image::save_pfm(std::string const& path) const
{
	std::ofstream of(path.c_str(), std::ios::out | std::ios::binary);

	if (!of) {
		std::cerr << "Cannot open " << path << " for writing." << std::endl;
		return;
	}

	// write header.
	of << "PF\n" << m_width << " " << m_height << "\n-1.0\n" << std::flush;

	std::vector<float> data_pfm(m_width * m_height * 3);
	for (int j = 0; j < m_width * m_height; ++j) {
		for (int i = 0; i < 3; ++i) {
			data_pfm[3*j+i] = getPixels()[j][i];
		}
	}
	of.write(reinterpret_cast<char const*>(&data_pfm[0]), 
		static_cast<std::streamsize>(m_width * m_height * 3 * sizeof(float)));

	if (!of) {
		std::cerr << "An error occured while writing " << path << std::endl;
	}

	of.close();
}
	
static void readCommentsAndEmptyLines(std::ifstream& file)
{
	std::string line;
	while (true) {
		std::streampos pos = file.tellg();
		getline(file, line);
		if (!(line.length() > 0 && line.at(0) == '#')) {
			file.seekg(pos);
			break;
		}
	}
}

void Image::load_pfm(std::string const& path)
{
	std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);

	if (!file || !file.is_open()) {
		std::cerr << "Cannot open " << path << " for reading." << std::endl;
		return;
	}

	std::string line;

	// read file type
	getline(file, line);
	readCommentsAndEmptyLines(file);

	// read width, height
	getline(file, line);
	std::sscanf(line.c_str(), "%i %i", &m_width, &m_height);
	getline(file, line); // read endian crap
	float endian;
	std::sscanf(line.c_str(), "%f", &endian);
	if (endian > 0) {
		std::cerr << "only little endian supported" << std::endl;
		m_width = 0;
		m_height = 0;
		return;
	}

	std::vector<float> data_pfm(m_width * m_height * 3 * sizeof(float));
	file.read(reinterpret_cast<char*>(&data_pfm[0]), 
		static_cast<std::streamsize>(sizeof(float) * 3 * m_width * m_height));
	if (!file) {
		std::cerr << "An error occured while reading " << path << "." << std::endl;
		return;
	}
	m_pixels.resize(m_width*m_height);

	for (int y = 0; y < m_height; ++y) 
	for (int x = 0; x < m_width; ++x) 
	{
		int offset = x + y * m_width;
		m_pixels[offset] = glm::vec4(
			data_pfm[3 * offset + 0],
			data_pfm[3 * offset + 1],
			data_pfm[3 * offset + 2],
			0.f);
	}
	file.close();
}

void Image::tonemap_01(float exposure, float gamma)
{
	glm::vec4 maxval {0.f, 0.f, 0.f, 0.f};
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			maxval = max(getPixel(x, y), maxval);
		}
	}
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			setPixel(x, y, pow(std::pow(2.f, exposure) * getPixel(x, y) / maxval, glm::vec4{1.f/gamma}));
		}
	}
}

/*
 * Writes a buffer of complex numbers into an image.
 *
 * - If rgba = true, the imaginary parst will be ignored and the real
 *	 part will be written to create an greyscale rgba image
 *
 * - If rgba = false, the data will be interpreted as an fourier spectrum
 *   and the real and imaginary parts will be written into the R channel and
 *   G channel respecively
 */
void Image::complex_to_image(std::vector<std::complex<float>> const& vec, Image* img, bool rgba)
{
	cg_assert(img);
	cg_assert(int(vec.size()) == img->getWidth()*img->getHeight());
	for (int j = 0; j < int(vec.size()); ++j)
	{
		const float r = vec[j].real();
		const float i = vec[j].imag();
		const float v = std::sqrt(r*r+i*i);
		if (rgba)
			img->getPixels()[j] = glm::vec4(v, v, v, 1.0f);
		else
			img->getPixels()[j] = glm::vec4(r, i, 0.0f, 1.0f);
	}
}
		
/*
 * Reads a buffer of complex numbers from an image.
 *
 * - If rgba = true, the color information of the image 
 *   will be averaged and written into the real part
 *   of the buffer.
 *
 * - If rgba = false, the data will be interpreted as an fourier spectrum
 *   and the R and G channel will be written into the real and imaginary
 *   part of the buffer respecively
 */
void Image::image_to_complex(Image const& img, std::vector<std::complex<float>>* vec, bool rgba)
{
	cg_assert(vec);
	cg_assert(int(vec->size()) == img.getWidth()*img.getHeight());
	for (int i = 0; i < int(vec->size()); ++i)
	{
		glm::vec4 const& d = img.getPixels()[i];
		if (rgba)
			(*vec)[i] = std::complex<float>((d[0]+d[1]+d[2])/3.f, 0);
		else
			(*vec)[i] = std::complex<float>(d[0], d[1]);
	}
}

