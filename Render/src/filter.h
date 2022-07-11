typedef	unsigned char	Pixel;

#ifdef __cplusplus
extern "C" {
#endif __cplusplus
// c - тип ильтрации:
//'b': box_filter
//'t': triangle_filter
//'q': bell_filter
//'B': B_spline_filter
//'h': filter
//'l': Lanczos3_filter
//'m': Mitchell_filter
int resample(Pixel*image_in,int dx_in,int dy_in,
			  Pixel* image_out, int dx_out,int dy_out, char c);


#ifdef __cplusplus
};
#endif __cplusplus
