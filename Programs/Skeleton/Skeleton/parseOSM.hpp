#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <iomanip>
#include <map>

#include "curl/curl.h"
#include "pugixml.hpp"

#include <chrono>


struct NodeStripped
{
	double lat;
	double lon;
};

using nodemap = std::unordered_map < long long, NodeStripped >;
using nodemap_entry = std::pair<long long, NodeStripped>;

struct Bounds
{
	double minlat = 0.;
	double maxlat = 0.;
	double minlon = 0.;
	double maxlon = 0.;

	Bounds() { ; }

	Bounds(double minlat, double maxlat, double minlon, double maxlon) : minlat(minlat), maxlat(maxlat), minlon(minlon), maxlon(maxlon) {}

	//vec2 getCenter() {}
};


struct Way
{
	std::string name = "n";
	std::vector<NodeStripped> nodes;
};


enum class EWayType : int
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
using waymap = std::map<EWayType, std::vector<Way>>;
using waymap_entry = std::pair<EWayType, std::vector<Way>>;

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
	size_t getNodeCount(const EWayType& type)
	{
		size_t c = 0;
		for (const auto& way : wayArrs.find(type)->second)
			c += way.nodes.size();
		return c;
	};
	size_t getNodeCount() {
		size_t c = 0;
		for (const auto& wayEntry : wayArrs)
			c += getNodeCount(wayEntry.first);
		return c;
	};
	size_t getWayCount()
	{
		size_t c = 0;
		for (const auto& wayEntry : wayArrs)
		{
			c += wayEntry.second.size();
		}
		return c;
	}
	void merge(MapData& other)
	{
		std::cout << getNodeCount() <<" + "<<other.getNodeCount() <<" = ";
		wayArrs.merge(other.wayArrs);
		for (auto& wayArr : wayArrs)
		{
			// Current EWayType
			const auto& t = wayArr.first;

			// My underlying vector
			auto& vec = wayArr.second;
			
			// Check if EWayType exists in other
			const auto iter = other.wayArrs.find(t);
			if (iter == other.wayArrs.end())
				continue;

			// The other's underlying vector
			const auto& vec_other = iter->second;

			// Merging
			vec.insert(vec.end(), vec_other.begin(), vec_other.end());
		}
		std::cout << getNodeCount() << "\n";
	}
};


bool getNodes(const std::string& path, MapData& mapData);

size_t WriteCallback(char* contents, size_t size, size_t nmemb, void* userp);

std::string* getOnlineOSM(const Bounds& bounds);

bool getNodesFromXML(const std::string& xml, MapData& maData);
