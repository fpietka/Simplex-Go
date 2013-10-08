package main

import (
	"fmt"
	"time"
	"math"
	"math/rand"
	"os"
	"encoding/binary"
)

const (
	hgrid int = 500 // x dimension of the grid
	vgrid int = 500 // y dimension of the grid
)

type color struct {
	//r=red, g=green, b=blue
	v[3] int
}


func main() {
	beginning := time.Now()

	//set the random seed
	rand.Seed(beginning.UnixNano())

	//make the empty array
	var grid[hgrid][vgrid] float64

	var min, max float64

	//now that the blank map is set up, unblank it
	grid, min, max = fillMap(grid, min, max)

	//now that we have an interesting map, create a .BMP file
	printMap(grid, min, max)

	end := time.Now()
	printPage(beginning, end)
}

func lerp(c1, c2 color, value float64) color {
	var tcolor = color{[3]int{0, 0, 0}}

	for g := 0; g < 3; g++ {
		if c1.v[g] > c2.v[g] {
			tcolor.v[g] = c2.v[g] + int((float64(c1.v[g] - c2.v[g]) * value))
		} else {
			tcolor.v[g] = c1.v[g] + int((float64(c2.v[g] - c1.v[g]) * value))
		}
	}

	return tcolor
}

func fillMap(grid[hgrid][vgrid] float64, min float64, max float64) ([hgrid][vgrid] float64, float64, float64) {
	//set up some variables
	var i, j, k int
	var octaves int = 16

	//these variables are used by the fBm part, but not extensively in the noise part
	var pixel_value, amplitude, frequency, gain, lacunarity float64
	//physics terms. gain affects the amplitude each octave, lacunarity affects the frequency
	gain = 0.65
	lacunarity = 2.0

	//these are all variables used only by the noise function, not the fBm part
	var disbx, disby float64 //distances to the three corners
	var dismx, dismy float64 //	b = bottom-left corner, m = middle corner, t = top-right corner
	var distx, disty float64
	var noiseb, noisem, noiset float64 //noise contributions from the three corners
	var tempdis, x, y float64
	var skew_value, unskew_value float64
	var general_skew float64 = (math.Sqrt(3.0) - 1.0) * 0.5 //these two are some complex math to convert between square and simplex space
	var general_unskew float64 = (3.0 - math.Sqrt(3.0)) / 6.0

	var cornerbx, cornerby int
	var cornermx, cornermy int
	var cornertx, cornerty int
	var gradb, gradm, gradt int //arrays should be used with all of these, but this is easier to read

	min = 100000.0
	max = -100000.0

	//set up the gradient table with 8 equally distributed angles around the unit circle
	var gradients[8][2] float64
	for i = 0; i < 8; i++ {
		gradients[i][0] = math.Cos(0.785398163 * float64(i)) // 0.785398163 is PI/4.
		gradients[i][1] = math.Sin(0.785398163 * float64(i))
	}

	//set up the random numbers table
	var permutations[256] int //make it as long as the largest dimension
	for i = 0; i < 256; i++ {
		permutations[i] = i //put each number in once
	}

	//randomize the random numbers table
	for i = 0; i < 256; i++  {
		j = int(random(256))
		k = permutations[i]
		permutations[i] = permutations[j]
		permutations[j] = k
	}

	//for each pixel...
	for i = 0; i < vgrid; i++ {
		for j = 0; j < hgrid; j++ {
			//get the value for this pixel by adding successive layers
			amplitude = 1.0
			frequency = 1.0 / float64(hgrid)
			pixel_value = 0.0

			for k = 0; k < octaves; k++ {
				//get the x and y values. These are values from the grid in normal (simplex) space
				x = float64(j) * frequency
				y = float64(i) * frequency


				//get the bottom-left corner of the simplex in skewed space
				skew_value = (x + y) * general_skew
				cornerbx = myfloor(x + skew_value)
				cornerby = myfloor(y + skew_value)

				//get the distance from the bottom corner in normal (simplex) space
				unskew_value = float64(cornerbx + cornerby) * general_unskew
				disbx = x - float64(cornerbx) + unskew_value
				disby = y - float64(cornerby) + unskew_value

				//get the middle corner in skewed space
				if disbx > disby {
					cornermx = 1 + cornerbx //lower triangle
					cornermy = cornerby
				} else {
					cornermx = cornerbx //upper triangle
					cornermy = 1 + cornerby
				}

				//get the top corner in skewed space
				cornertx = 1 + cornerbx
				cornerty = 1 + cornerby

				//get the distance from the other two corners
				dismx = disbx - float64(cornermx - cornerbx) + general_unskew
				dismy = disby - float64(cornermy - cornerby) + general_unskew

				distx = disbx - 1.0 + general_unskew + general_unskew
				disty = disby - 1.0 + general_unskew + general_unskew

				//get the gradients indices
				gradb = permutations[(cornerbx + permutations[cornerby & 255]) & 255] & 7
				gradm = permutations[(cornermx + permutations[cornermy & 255]) & 255] & 7
				gradt = permutations[(cornertx + permutations[cornerty & 255]) & 255] & 7

				//get the noise from each corner using an attenuation function
				//first the bottom corner
				tempdis = 0.5 - disbx * disbx - disby * disby
				if tempdis < 0.0 {
					noiseb = 0.0
				} else {
					noiseb = math.Pow(tempdis, 4.0) * dotproduct(gradients[gradb], disbx, disby)
				}

				//then the middle corner
				tempdis = 0.5 - dismx * dismx - dismy * dismy
				if tempdis < 0.0 {
					noisem = 0.0
				} else {
					noisem = math.Pow(tempdis, 4.0) * dotproduct(gradients[gradm], dismx, dismy)
				}

				//	last the top corner
				tempdis = 0.5 - distx * distx - disty * disty
				if tempdis < 0.0 {
					noiset = 0.0
				} else {
					noiset = math.Pow(tempdis, 4.0) * dotproduct(gradients[gradt], distx, disty)
				}

				//finally, add it in and adjust for the next layer
				//	notice that no interpolation is needed, just straight summation
				pixel_value += (noiseb + noisem + noiset) * amplitude

				amplitude *= gain
				frequency *= lacunarity
			}

			//put it in the map
			grid[j][i] = pixel_value

			//do some quick checks
			if pixel_value < min {
				min = pixel_value
			} else if pixel_value > max {
				max = pixel_value
			}
		}
	}

	return grid, min, max
}

func printMap(grid[hgrid][vgrid] float64, min float64, max float64) {
	//set up some variables
	var diff float64 = max - min
	var flood float64 = 0.5 //flood level
	var mount float64 = 0.85 //mountain level

	flood *= diff
	mount *= diff

	var i, j, k int

	//these can be changed for interesting results
	var landlow  = color{[3]int{0, 64, 0}}
	var landhigh = color{[3]int{116, 182, 133}}
	var waterlow = color{[3]int{55, 0, 0}}
	var waterhigh = color{[3]int{106, 53, 0}}
	var mountlow = color{[3]int{147, 157, 167}}
	var mounthigh = color{[3]int{226, 223, 216}}

	//3.0 output to file
	//3.1 Begin the file
	//3.1.1 open output file
	out, err := os.Create("test.bmp")
	if err != nil {
		fmt.Printf("Target file opening error\n")
		panic(err)
	}

	//3.1.2 copy the header
	//3.1.2.1 magic number
	out.Write(char(66))
	out.Write(char(77))

	//3.1.2.2 filsize/unused space
	for i = 0; i < 8; i++ {
		out.Write(char(0))
	}

	//3.1.2.3 data offset
	out.Write(char(54))

	//3.1.2.4 unused space
	for i = 0; i < 3; i++ {
		out.Write(char(0))
	}

	//3.1.2.5 header size
	out.Write(char(40))

	//3.1.2.6 unused space
	for i = 0; i < 3; i++ {
		out.Write(char(0))
	}

	//3.1.2.7 file width (trickier)
	out.Write(char(hgrid % 256))
	out.Write(char((hgrid >> 8) % 256))
	out.Write(char((hgrid >> 16) % 256))
	out.Write(char((hgrid >> 24) % 256))

	//3.1.2.8 file height (trickier)
	out.Write(char(vgrid % 256))
	out.Write(char((vgrid >> 8) % 256))
	out.Write(char((vgrid >> 16) % 256))
	out.Write(char((vgrid >> 24) % 256))

	//3.1.2.9 color planes
	out.Write(char(1))
	out.Write(char(0))

	//3.1.2.10 bit depth
	out.Write(char(24))

	//3.1.2.11 the rest
	for i = 0; i < 25; i++ {
		out.Write(char(0))
	}

	//3.2 put in the elements of the array
	var newcolor = color{[3]int{0, 0, 0}}
	for i = (vgrid - 1); i >= 0; i-- { //bitmaps start with the bottom row, and work their way up...
		for j = 0; j < hgrid; j++ { //...but still go left to right
			grid[j][i] -= min
			if grid[j][i] < flood {
				//if this point is below the floodline...
				newcolor = lerp(waterlow, waterhigh, grid[j][i] / flood)
			} else if grid[j][i] > mount {
				//if this is above the mountain line...
				newcolor = lerp(mountlow, mounthigh, (grid[j][i] - mount) / (diff - mount))
			} else {
				//if this is regular land
				newcolor = lerp(landlow, landhigh, (grid[j][i] - flood) / (mount - flood))
			}

			out.Write(char(newcolor.v[0])) //blue
			out.Write(char(newcolor.v[1])) //green
			out.Write(char(newcolor.v[2])) //red
		}
		//round off the row
		for k = 0; k < (hgrid % 4); k++ {
			out.Write(char(0))
		}
	}

	//3.3 end the file
	out.Close()
}

func char(value int) ([]byte) {
	buf := make([]byte, binary.MaxVarintLen64)
	binary.PutUvarint(buf, uint64(value))

	final := make([]byte, 1)
	final[0] = buf[0]
	return final
}

func random(max float64) float64 {
	var r int
	var s float64

	r = rand.Int()
	s = float64(r & 0x7fff) / float64(0x7fff)

	return (s * max)
}

func printPage(beginning, end time.Time) {
	duration := end.Sub(beginning)
	fmt.Printf("Content-Type: text/html\n\n")
	fmt.Printf("<html><head><title>FTG Page</title></head>\n")
	fmt.Printf("<body>\n")
	fmt.Printf("<h2>Fractal Terrain Generator Page</h2>\n")
	fmt.Printf("<img src=\"test.bmp\" /><br />\n")
	fmt.Printf("This took " + duration.String() + " seconds to create.<br />\n")
	fmt.Printf("</body>\n")
	fmt.Printf("</html>\n")
}

func myfloor(value float64) (result int) {
	if value >= 0 {
		result = int(value)
	} else {
		result = int(value) - 1
	}
	return
}

func dotproduct(grad [2]float64, x, y float64) float64 {
	return (grad[0] * x + grad[1] * y)
}
