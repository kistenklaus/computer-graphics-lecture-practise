#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <memory>
#include <complex>
//#include <omp.h>

class Image
{
public:
	Image();
	Image(int width, int height);

	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }

	void setSize(int width, int height);

	const glm::vec4& getPixel(int i, int j) const;
	const glm::vec4* getPixels() const;

	void setPixel(int i, int j, const glm::vec4& pixel);
	glm::vec4* getPixels();

	float const* raw_data() const;
	float* raw_data();
	void clear(glm::vec4 const& color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

	void save(std::string const& path, float gamma) const;
	void load(std::string const& path, float gamma);
	
	void save_pfm(std::string const& path) const;
	void load_pfm(std::string const& path);

	void tonemap_01(float exposure, float gamma);

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
	static void complex_to_image(std::vector<std::complex<float>> const& vec, Image* img, bool rgba);
	
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
	static void image_to_complex(Image const& img, std::vector<std::complex<float>>* vec, bool rgba);

protected:
	int m_width;
	int m_height;
	std::vector<glm::vec4> m_pixels;

};

struct DiscreteFourier2D
{
	static void reconstruct(
		int M, int N,
		std::complex<float> const* spectrum,
		std::complex<float>	* reconstruction);

};




