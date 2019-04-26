
# ITCT Jpeg baseline Decoder / Encoder

## Compilation
It's recommended to use g++ version 8 to compile.

```bash
make
```

## Execution
* Decoder:

```bash
./decode -s [path to source] -f [file format (bmp, ppm)] -d [path to destination]
```

The ```-s``` argument is required. The others are optional, where ```-f``` is defaulted to ```bmp``` and ```-d``` is defaulted to ```output.bmp```.


* Encoder:

```bash
./encode -s [path to source] -f [file format (bmp, ppm)] -d [path to destination] -p [subsampling method (0, 1)] -r [quality (low, high, lossless)]
```
The ```-s``` argument is required. The others are optional, where ```-f``` is defaulted to ```bmp```,  ```-d``` is defaulted to ```output.jpg```, ```-p``` is defaulted to 0 and ```-r``` is defaulted to ```low```. If the ```-p``` argument is set to 0, then 1x1 : 1x1 : 1x1 subsampling will be used, otherwise 2x2 : 1x1 : 1x1 subsampling is used instead. (It's recommended to use ```ppm``` in encoder since some headers of ```bmp``` is not supported.)

## Architecture
The 2D-DCT in encoding is implemented with two phases of 1D-DCT: first apply 1D-DCT on all rows, then apply 1D-DCT on columns. The 1D-DCT is optimized with Chen's 8-point DCT butterfly algorithm.

Three pairs of quantizers is used which corresponds to different quality.

![equation](https://latex.codecogs.com/gif.latex?%5Cbegin%7Bbmatrix%7D%2016%20%26%2011%20%26%2010%20%26%2016%20%26%2024%20%26%2040%20%26%2051%20%26%2061%20%5C%5C%2012%20%26%2012%20%26%2014%20%26%2019%20%26%2026%20%26%2058%20%26%2060%20%26%2055%20%5C%5C%2014%20%26%2013%20%26%2016%20%26%2024%20%26%2040%20%26%2067%20%26%2069%20%26%2056%20%5C%5C%2014%20%26%2017%20%26%2022%20%26%2029%20%26%2051%20%26%2087%20%26%2080%20%26%2062%20%5C%5C%2018%20%26%2022%20%26%2037%20%26%2056%20%26%2068%20%26%20109%20%26%20103%20%26%2077%20%5C%5C%2024%20%26%2035%20%26%2055%20%26%2064%20%26%2081%20%26%20104%20%26%20113%20%26%2092%20%5C%5C%2049%20%26%2064%20%26%2078%20%26%2087%20%26%20103%20%26%20121%20%26%20120%20%26%20101%20%5C%5C%2072%20%26%2092%20%26%2095%20%26%2098%20%26%20112%20%26%20100%20%26%20103%20%26%2099%20%5C%5C%20%5Cend%7Bbmatrix%7D)

