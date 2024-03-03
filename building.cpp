/*building.cpp*/

//
// A building in the Open Street Map.
//
// Prof. Joe Hummel
// Shreeniket Bendre
// Northwestern University
// CS 211
//

#include <iostream>
#include <utility>

#include "building.h"
#include "busstop.h"
#include "busstops.h"
#include "dist.h"
#include "curl_util.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

//
// constructor
//
Building::Building(long long id, string name, string streetAddr)
    : ID(id), Name(name), StreetAddress(streetAddr)
{
  //
  // the proper technique is to use member initialization list above,
  // in the same order as the data members are declared:
  //
  // this->ID = id;
  // this->Name = name;
  // this->StreetAddress = streetAddr;

  // vector is default initialized by its constructor
}

//
// print
//
// prints information about a building --- id, name, etc. -- to
// the console. The function is passed the Nodes for searching
// purposes.
//
void Building::print(Nodes &nodes, BusStops &busstops, CURL *curl)
{
  cout << this->Name << endl;
  cout << "Address: " << this->StreetAddress << endl;
  cout << "Building ID: " << this->ID << endl;
  cout << "# perimeter nodes: " << this->NodeIDs.size() << endl;
  double latLoc = this->getLocation(nodes).first;
  double lonLoc = this->getLocation(nodes).second;
  cout << "Location: (" << latLoc << ", " << lonLoc << ")" << endl;

  BusStop sb = busstops.MapBusStops[this->indexOfClosestBusStop(busstops, "Southbound", latLoc, lonLoc)];
  BusStop nb = busstops.MapBusStops[this->indexOfClosestBusStop(busstops, "Northbound", latLoc, lonLoc)];

  cout << "Closest southbound bus stop:" << endl;
  cout << "  " << sb.ID << ": " << sb.StopName << ", bus #" << sb.Route << ", " << sb.Location << ", "
       << distBetween2Points(sb.Lat, sb.Lon, latLoc, lonLoc) << " miles" << endl;
  this->makeAndPrintRequest(curl, sb);

  cout << "Closest northbound bus stop:" << endl;
  cout << "  " << nb.ID << ": " << nb.StopName << ", bus #" << nb.Route << ", " << nb.Location << ", "
       << distBetween2Points(nb.Lat, nb.Lon, latLoc, lonLoc) << " miles" << endl;
  this->makeAndPrintRequest(curl, nb);
}

//
// indexOfClosestBusStop
//
// Returns index of closest bus stop. Must be
// given BusStops vector, intended direction
// (ex. Southbound, Northbound), and latitude
// longitude of building
//
long long Building::indexOfClosestBusStop(BusStops &busstops, string dir, double latLoc, double lonLoc)
{
  vector<double> sizes;
  for (BusStop bs : busstops.MapBusStops)
  { // iterates thru each busstop to append distance to sizes vector
    // bs.Lat
    sizes.push_back(distBetween2Points(bs.Lat, bs.Lon, latLoc, lonLoc));
  }
  long long indexLow = -1;
  bool cont = true;
  while (cont){//checks for first occurence of desired direction
    indexLow++;
    if (busstops.MapBusStops[indexLow].Direction == dir){
      cont = false; //found first occurence, break loop
    }
    if (indexLow == (int)sizes.size()){
      cout << "  **CRITICAL ERROR: NO NORTH OR SOUTH BOUND**" << endl;
      return -1;
    }
  }
  for (int i = 0; i < (int)sizes.size(); i++)
  { // iterates thru sizes vector to find smallest size with given direction
    if (sizes[i] < sizes[indexLow] && busstops.MapBusStops[i].Direction == dir)
    {
      indexLow = i;
    }
  }

  return indexLow;
}

//
// makeAndPrintRequest()
//
// Given a curl and a busstop, will make predicition request
// and properly print JSON
//

void Building::makeAndPrintRequest(CURL *curl, BusStop &busstop)
{
  //defines url to make
  string url = "https://ctabustracker.com/bustime/api/v2/getpredictions?key=hjrs9XZsn2XTjrTpEGwUsESqj&rt=";
  url += to_string(busstop.Route);
  url += "&stpid=";
  url += to_string(busstop.ID);
  url += "&format=json";
  string responseStr;

  bool responseBool = callWebServer(curl, url, responseStr);
  if (!responseBool)
  {
    cout << "  <<bus predictions unavailable, call failed>>" << endl;
    return;
  }

  auto jsondata = json::parse(responseStr);
  auto bus_response = jsondata["bustime-response"];
  auto predictions = bus_response["prd"];

  if (predictions.size() <= 0)
  {
    cout << "  <<no predictions available>>" << endl;
  }
  else
  {
    for (auto &M : predictions)
    {
      try // EXCEPTION HANDLING STEP 7
      {
        int vehicleNum = stoi(M["vid"].get_ref<std::string &>());
        int route = stoi(M["rt"].get_ref<std::string &>());
        string dir = M["rtdir"].get_ref<std::string &>();
        int pred = stoi(M["prdctdn"].get_ref<std::string &>());

        cout << "  vehicle #" << vehicleNum << " on route " << route
             << " travelling " << dir << " to arrive in " << pred << " mins"
             << endl;
      }
      catch (exception &e)
      {
        cout << "  error" << endl;
        cout << "  malformed CTA response, prediction unavailable"
             << " (error: " << e.what() << ")" << endl;
      }
    }
  }
}

//
// adds the given nodeid to the end of the vector.
//
void Building::add(long long nodeid)
{
  this->NodeIDs.push_back(nodeid);
}

//
// gets the center (lat, lon) of the building
// based on the nodes that form the perimeter
//
pair<double, double> Building::getLocation(const Nodes &nodes)
{
  double latTot = 0;
  double lonTot = 0;
  for (long long nodeid : this->NodeIDs)
  {
    double lat = 0;
    double lon = 0;
    bool e = false;
    e = nodes.find(nodeid, lat, lon, e);
    latTot += lat;
    lonTot += lon;
  }
  latTot = latTot / this->NodeIDs.size();
  lonTot = lonTot / this->NodeIDs.size();
  return make_pair(latTot, lonTot);
}

// cout << "Nodes:" << endl;
// for (long long nodeid : this->NodeIDs)
// {
//   cout << "  " << nodeid << ": ";

//   double lat = 0.0;
//   double lon = 0.0;
//   bool entrance = false;

//   bool found = nodes.find(nodeid, lat, lon, entrance);

//   if (found) {
//     cout << "(" << lat << ", " << lon << ")";

//     if (entrance)
//       cout << ", is entrance";

//     cout << endl;
//   }
//   else {
//     cout << "**NOT FOUND**" << endl;
//   }
// }//for