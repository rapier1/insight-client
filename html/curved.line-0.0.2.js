/* 
 
	Simple Javascript Curved Line function for use with Google Maps Api 
	
	author: Daniel Nanovski
	modifications: Coen de Jong
	version: 0.0.2 (Beta)
	website: http://curved_lines.overfx.net/
	
	License:
	Copyright (c) 2012 Daniel Nanovski, http://overfx.net/

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the
	"Software"), to deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to
	permit persons to whom the Software is furnished to do so, subject to
	the following conditions:
	
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
*/
evenOdd = 0;
var LineCordinates;

function curved_line_generate(Options) {

	LineCordinates = new Array();

	var Options = Options || {};
	var LatStart = Options.latStart || Options.path[0].lat() || null;
	var LngStart = Options.lngStart || Options.path[0].lng() || null;
	var LatEnd = Options.latEnd || Options.path[1].lat() || null;
	var LngEnd = Options.lngEnd || Options.path[1].lng() || null;
	var Color = Options.strokeColor || "#FF0000";
	var Opacity = Options.strokeOpacity || 1;
	var Weight = Options.strokeWeight || 3;
	var GapWidth = Options.gapWidth || 0;
	var Horizontal = Options.horizontal;
	var Multiplier = Options.multiplier || 1;
	var Resolution = Options.resolution || 0.1;
	var Map = Options.map; //Options.Map || null;
	var Icons = Options.icons || null;
	
	//console.log(Options);

	if(Horizontal == undefined) {
	
		Horizontal = true;
	
	}
	
	var LastLat = LatStart;
	var LastLng = LngStart;
	
	var PartLat;
	var PartLng;
		
	var Points = new Array();
	var PointsOffset = new Array();
	
	for(point = 0; point <= 1; point += Resolution)
	{
		Points.push(point);
		offset = (0.6 * Math.sin((Math.PI * point / 1)));
		PointsOffset.push(offset);
	}
			
	var OffsetMultiplier = 0;
	
	if(Horizontal == 1) {
	
		var OffsetLenght = (LngEnd - LngStart) * 0.1;
	
	} else {
		
		var OffsetLenght = (LatEnd - LatStart) * 0.1;
		
	}
			 
	for(var i = 0; i < Points.length; i++) {
		
		if(i == 4) {
			
			OffsetMultiplier = 1.5 * Multiplier;
			
		}
		
		if(i >= 5) {
			
			OffsetMultiplier = (OffsetLenght * PointsOffset[i]) * Multiplier;
			
		} else {
		
			OffsetMultiplier = (OffsetLenght * PointsOffset[i]) * Multiplier;
			
		}
		
		if(Horizontal == 1) {
		
			PartLat = (LatStart + ((LatEnd - LatStart) * Points[i])) + OffsetMultiplier;
			PartLng = (LngStart + ((LngEnd - LngStart) * Points[i]));
		
		} else {
			
			PartLat = (LatStart + ((LatEnd - LatStart) * Points[i]));
			PartLng = (LngStart + ((LngEnd - LngStart) * Points[i])) + OffsetMultiplier;
			
		}
		
		//curved_line_create_segment(LastLat, LastLng, PartLat, PartLng, Color, Opacity, Weight, GapWidth, Map, Icons);
		LineCordinates.push(new google.maps.LatLng(LastLat, LastLng));

		LastLat = PartLat;
		LastLng = PartLng;
	
	}
	
	//curved_line_create_segment(LastLat, LastLng, LatEnd, LngEnd, Color, Opacity, Weight, GapWidth, Map, Icons);
	LineCordinates.push(new google.maps.LatLng(LastLat, LastLng));

	return curved_line_create_segment(LastLat, LastLng, LatEnd, LngEnd, Color, Opacity, Weight, GapWidth, Map, Icons);
}

function curved_line_create_segment(LatStart, LngStart, LatEnd, LngEnd, Color, Opacity, Weight, GapWidth, Map, Icons) {
	
	evenOdd++;
	
	if(evenOdd % (GapWidth+1))
		return;
		
	//var LineCordinates = new Array();
	
	//LineCordinates[0] = new google.maps.LatLng(LatStart, LngStart);
	//LineCordinates[1] = new google.maps.LatLng(LatEnd, LngEnd);
	
	var Line = new google.maps.Polyline({
		path: LineCordinates,
		geodesic: false,
		strokeColor: Color,
		strokeOpacity: Opacity,
		strokeWeight: Weight,
	    	icons: Icons
	}); 
	Line.setMap(Map);	
	return Line;
}
