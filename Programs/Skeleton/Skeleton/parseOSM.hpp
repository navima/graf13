#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <iomanip>
#include <map>

#include <chrono>


struct NodeStripped
{
	double lat;
	double lon;
};

typedef std::unordered_map < long long, NodeStripped > nodemap;
typedef std::pair<long long, NodeStripped> nodemap_entry;

struct Bounds
{
	double minlat, maxlat, minlon, maxlon;

	Bounds() { ; }

	Bounds(double minlat, double maxlat, double minlon, double maxlon) : minlat(minlat), maxlat(maxlat), minlon(minlon), maxlon(maxlon) {}

	//vec2 getCenter() {}
};


struct Way
{
	std::string name = "n";
	std::vector<NodeStripped> nodes;
};


enum EWayType : int
{
	defaultt = 0,
	building = 1,
	boundary = 2,
	route_highway = 3,
	route_railway = 4,
	path = 5,
	wetland = 6,
	grassland,
	water,
	sand
};

const std::unordered_map<std::string, EWayType> attr_val_to_EWayType({
	{"living_street",    EWayType::route_highway},
	{"primary",    EWayType::route_highway},
	{"primary_link",    EWayType::route_highway},
	{"residential",    EWayType::route_highway},
	{"road",    EWayType::route_highway},
	{"secondary",    EWayType::route_highway},
	{"tertiary",    EWayType::route_highway},
	{"unclassified",    EWayType::route_highway},

	{"wetland", EWayType::wetland},

	{"grassland", EWayType::grassland},

	{"water", EWayType::water},

	{"beach", EWayType::sand},
	{"sand", EWayType::sand},

	{"path",    EWayType::path},
	{"track",    EWayType::path},
	{"hiking",    EWayType::path},

	{"railway",    EWayType::route_railway},
	{"train",   EWayType::route_railway} });

const std::unordered_map<std::string, EWayType> attr_name_to_EWayType({
	{"building",EWayType::building },
	{"boundary",EWayType::boundary},
	{ "railway",EWayType::route_railway } });


struct MapData;
typedef std::map<EWayType, std::vector<Way>> waymap;
typedef std::pair<EWayType, std::vector<Way>> waymap_entry;

struct MapData
{
	Bounds bounds;
	waymap wayArrs;

	void push_back(const EWayType& type, const Way& way)
	{
		auto findIter = wayArrs.find(type);
		if (findIter != wayArrs.end())
			findIter->second.push_back(way);
		else
			wayArrs.insert({ type, { way } });
	};
	unsigned long getNodeCount(const EWayType& type)
	{
		unsigned long c = 0;
		for (const auto& way : wayArrs.find(type)->second)
			c += way.nodes.size();
		return c;
	};
	unsigned long getNodeCount() {
		unsigned long c = 0;
		for (const auto& wayEntry : wayArrs)
			c += getNodeCount(wayEntry.first);
		return c;
	};
	unsigned long getWayCount()
	{
		unsigned long c = 0;
		for (const auto& wayEntry : wayArrs)
		{
			c += wayEntry.second.size();
		}
		return c;
	}
};


bool getNodes(const std::string& path, MapData& mapData);
