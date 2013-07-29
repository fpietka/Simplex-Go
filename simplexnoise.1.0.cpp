/*
Copyright (c) 2009, Travis Archer
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution.

Neither the name of Morningside College nor the names of its contributors may be used to endorse or promote products derived
from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/***************************************************************
*Fractal Terrain Template
*created by Travis Archer
*
*This program provides a template where any algorithm can be
*   plugged in to create a fractal terrain map without
*   writing the boring parts.
*
*1.0 initalizes variables (there are many)
*2.0 the map is created, this is the interesting part
*3.0 the map is outputted to a bitmap file. It should be noted
*	that the map is only a series of numbers before this step.
*	This step finds the flood level, and creates colors
*	accordingly. This part may be as interesting as 2.0 to some
*4.0 a webpage is created, displaying the bitmap
****************************************************************/

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <ctime>

using namespace std;

/*****************************************FILENAME DEFINITION***/
const char oname[16]="test.bmp";

/*******************************************STRUCT DEFINITION***/
struct color{
	//v[0]=red, v[1]=green, v[2]=blue
	unsigned char v[3];

	color(unsigned char r, unsigned char g, unsigned char b){
		v[0]=r;
		v[1]=g;
		v[2]=b;
	}
};

/********************************************SIZE DEFINITIONS***/
const unsigned hgrid = 500,//x dimension of the grid
			   vgrid = 500;//y dimension of the grid

/*****************************************FUNCTION PROTOTYPES***/
float random(float r);
color lerp(color c1, color c2, float value);//LERP = Linear intERPolation

void fillMap(float map[][vgrid], float &min, float &max);// this is the algorithm part (the interesting part)
void printMap(float map[][vgrid], float min, float max);//bitmap part
void printPage(float beginning, float end);//webpage part

//the remaining functions are all used by the Simplex Noise function, not the template
int   myfloor(float value);
float dotproduct(float grad[], float x, float y);

/********************************************************MAIN***/
int main(){
//1.0 initialize some variables
	float beginning = clock(),//these two are used to time our algorithm
		   end;

	srand((unsigned)beginning);//set the random seed

	float map[hgrid][vgrid];//make the empty array

	float min,max; //used to normalize the map

	//now that the blank map is set up, unblank it
	fillMap(map, min, max);

	//now that we have an interesting map, create a .BMP file
	printMap(map, min, max);

	//finally, output a webpage
	end = clock();//set it to now
	printPage(beginning, end);
}


/***************************************FUNCTION DECLARATIONS***/
color lerp(color c1, color c2, float value){
	color tcolor(0,0,0);

	for (int g=0;g<3;g++){
		if (c1.v[g]>c2.v[g])
			tcolor.v[g]=c2.v[g]+(unsigned char)(float(c1.v[g]-c2.v[g])*value);

		else
			tcolor.v[g]=c1.v[g]+(unsigned char)(float(c2.v[g]-c1.v[g])*value);
	}

	return (tcolor);
}

float random(float max){
	int r;
    float s;

	r = rand();
    s = (float)(r & 0x7fff)/(float)0x7fff;

    return (s * max);
}

void fillMap(float map[][vgrid], float &min, float &max)
{
	//set up some variables
	int i,j,k,
		octaves = 16;

	//these variables are used by the fBm part, but not extensively in the noise part
	float pixel_value,
		  amplitude, frequency,
		  gain = 0.65f, lacunarity = 2.0f;//physics terms. gain affects the amplitude each octave, lacunarity affects the frequency

	//these are all variables used only by the noise function, not the fBm part
	float disbx, disby,//distances to the three corners
		  dismx, dismy,//	b = bottom-left corner, m = middle corner, t = top-right corner
		  distx, disty,
		  noiseb,noisem,noiset,//noise contributions from the three corners
		  tempdis,x,y,
		  skew_value,unskew_value,
		  general_skew = (sqrt(3.0f)-1.0f)*0.5f, //these two are some complex math to convert between square and simplex space
		  general_unskew = (3.0f-sqrt(3.0f))/6.0f;

	int cornerbx, cornerby,
		cornermx, cornermy,
		cornertx, cornerty,
		gradb, gradm, gradt;//arrays should be used with all of these, but this is easier to read

	min = 100000.0f;
	max = -100000.0f;

	//set up the gradient table with 8 equally distributed angles around the unit circle
	float gradients[8][2];
	for (i=0;i<8;++i)
	{
		gradients[i][0] = cos(0.785398163f * (float)i);// 0.785398163 is PI/4.
		gradients[i][1] = sin(0.785398163f * (float)i);
	}

	//set up the random numbers table
	int permutations[256]; //make it as long as the largest dimension
	for (i=0;i<256;++i)
		permutations[i] = i;//put each number in once

	//randomize the random numbers table
	for (i=0;i<256;++i)
	{
		j = (int)random(256);
		k = permutations[i];
		permutations[i] = permutations[j];
		permutations[j] = k;
	}

	//for each pixel...
	for (i=0;i<vgrid;++i)
	{
		for (j=0;j<hgrid;++j)
		{
			//get the value for this pixel by adding successive layers
			amplitude = 1.0f;
			frequency = 1.0f / (float)hgrid;
			pixel_value = 0.0f;

			for (k = 0; k < octaves; ++k)
			{
				//get the x and y values. These are values from the grid in normal (simplex) space
				x = (float)j * frequency;
				y = (float)i * frequency;


				//get the bottom-left corner of the simplex in skewed space
				skew_value = (x + y) * general_skew;
				cornerbx = myfloor(x + skew_value);
				cornerby = myfloor(y + skew_value);

				//get the distance from the bottom corner in normal (simplex) space
				unskew_value = float(cornerbx + cornerby) * general_unskew;
				disbx = x - (float)cornerbx + unskew_value;
				disby = y - (float)cornerby + unskew_value;

				//get the middle corner in skewed space
				if (disbx > disby)
				{
					cornermx = 1 + cornerbx;//lower triangle
					cornermy = cornerby;
				}
				else
				{
					cornermx = cornerbx;//upper triangle
					cornermy = 1 + cornerby;
				}

				//get the top corner in skewed space
				cornertx = 1 + cornerbx;
				cornerty = 1 + cornerby;

				//get the distance from the other two corners
				dismx = disbx - float(cornermx-cornerbx) + general_unskew;
				dismy = disby - float(cornermy-cornerby) + general_unskew;

				distx = disbx - 1.0f + general_unskew + general_unskew;
				disty = disby - 1.0f + general_unskew + general_unskew;

				//get the gradients indices
				gradb = permutations[(cornerbx + permutations[cornerby & 255]) & 255] & 7;
				gradm = permutations[(cornermx + permutations[cornermy & 255]) & 255] & 7;
				gradt = permutations[(cornertx + permutations[cornerty & 255]) & 255] & 7;

				//get the noise from each corner using an attenuation function
				//	first the bottom corner
				tempdis = 0.5f - disbx * disbx - disby * disby;
				if (tempdis < 0.0f)
					noiseb = 0.0f;
				else
					noiseb = pow(tempdis,4.0f) * dotproduct(gradients[gradb],disbx,disby);

				//	then the middle corner
				tempdis = 0.5f - dismx * dismx - dismy * dismy;
				if (tempdis < 0.0f)
					noisem = 0.0f;
				else
					noisem = pow(tempdis,4.0f) * dotproduct(gradients[gradm],dismx,dismy);

				//	last the top corner
				tempdis = 0.5f - distx * distx - disty * disty;
				if (tempdis < 0.0f)
					noiset = 0.0f;
				else
					noiset = pow(tempdis,4.0f) * dotproduct(gradients[gradt],distx,disty);

				//finally, add it in and adjust for the next layer
				//	notice that no interpolation is needed, just straight summation
				pixel_value += (noiseb + noisem + noiset) * amplitude;

				amplitude *= gain;
				frequency *= lacunarity;
			}

			//put it in the map
			map[j][i] = pixel_value;

			//do some quick checks
			if (pixel_value < min)
				min = pixel_value;

			else if (pixel_value > max)
				max = pixel_value;
		}
	}
}

void printMap(float map[][vgrid], float min, float max)
{
//set up some variables
	float diff = max-min,
		  flood=0.5f,//flood level
		  mount=0.85f;//mountain level

	flood*=diff;
	mount*=diff;

	int i,j,k;

	//these can be changed for interesting results
	color landlow(0,64,0),
		  landhigh(116,182,133),
		  waterlow(55,0,0),
		  waterhigh(106,53,0),
		  mountlow(147,157,167),
		  mounthigh(226,223,216);

//3.0 output to file
	//3.1 Begin the file
	//3.1.1 open output file
	ofstream out;
	out.open(oname, ofstream::binary);
	if (!(out.is_open())){
		cout << "Target file opening error"<<endl;
		exit(0);
	}

	//3.1.2 copy the header
		//3.1.2.1 magic number
		out.put(char(66));
		out.put(char(77));

		//3.1.2.2 filsize/unused space
		for (i=0;i<8;i++)
			out.put(char(0));

		//3.1.2.3 data offset
		out.put(char(54));

		//3.1.2.4 unused space
		for (i=0;i<3;i++)
			out.put(char(0));

		//3.1.2.5 header size
		out.put(char(40));

		//3.1.2.6 unused space
		for (i=0;i<3;i++)
			out.put(char(0));

		//3.1.2.7 file width (trickier)
		out.put(char(hgrid%256));
		out.put(char((hgrid>>8)%256));
		out.put(char((hgrid>>16)%256));
		out.put(char((hgrid>>24)%256));

		//3.1.2.8 file height (trickier)
		out.put(char(vgrid%256));
		out.put(char((vgrid>>8)%256));
		out.put(char((vgrid>>16)%256));
		out.put(char((vgrid>>24)%256));

		//3.1.2.9 color planes
		out.put(char(1));
		out.put(char(0));

		//3.1.2.10 bit depth
		out.put(char(24));

		//3.1.2.11 the rest
		for (i=0;i<25;i++)
			out.put(char(0));

	//3.2 put in the elements of the array
	color newcolor(0,0,0);
	for (i=(vgrid-1);i>=0;i--){//bitmaps start with the bottom row, and work their way up...
		for (j=0;j<hgrid;j++){//...but still go left to right
			map[j][i]-=min;
			//if this point is below the floodline...
			if (map[j][i]<flood)
				newcolor=lerp(waterlow,waterhigh,map[j][i]/flood);

			//if this is above the mountain line...
			else if (map[j][i]>mount)
				newcolor=lerp(mountlow,mounthigh,(map[j][i]-mount)/(diff-mount));

			//if this is regular land
			else
				newcolor=lerp(landlow,landhigh,(map[j][i]-flood)/(mount-flood));

			out.put(char(newcolor.v[0]));//blue
			out.put(char(newcolor.v[1]));//green
			out.put(char(newcolor.v[2]));//red
		}
		//round off the row
		for (k=0;k<(hgrid%4);k++)
			out.put(char(0));
	}

	//3.3 end the file
	out.close();
}

void printPage(float beginning, float end)
{
    //float diff = ((end - beginning) / 1000.0F ) * 1000;
    float diff = (round(end - beginning)) / 1000000;
	cout<<"Content-Type: text/html\n\n"
		<<"<html><head><title>FTG Page</title></head>\n"
		<<"<body>\n"
		<<"<h2>Fractal Terrain Generator Page</h2>\n"
		<<"<img src=\"test.bmp\" /><br />\n"
		<<"This took " << diff << " seconds to create.<br />\n"
		<<"Press F5 or your browser's refresh button to see a new image.\n"
		<<"</body>\n"
		<<"</html>\n";
}

int myfloor(float value)
{
	return (value >= 0 ? (int)value : (int)value-1);
}

float dotproduct(float grad[], float x, float y)
{
	return (grad[0] * x + grad[1] * y);
}
