#pragma once

#include "parseOSM.hpp"
#include "framework.h"

class MapDataG : public MapData
{
public:
	std::vector<std::vector<colored_point>> vertArr;

	void draw(const EWayType& type, const GLuint& vbo)
	{
		auto vertices = std::vector<colored_point>();

		auto wayiter = wayArrs.find(type);
		if (wayiter != wayArrs.end());
		{
			auto& wayType = wayiter->first;
			auto& wayArr = wayiter->second;

			vec3 wayColour;
			switch (wayType)
			{
			case EWayType::building:
				wayColour = { .65f, .65f, .65f };
				break;
			case EWayType::route_highway:
				wayColour = { .8f,.8f,.2f };
				break;
			case EWayType::route_railway:
				wayColour = { .6f,.1f,.1f };
				break;
			case EWayType::boundary:
				wayColour = { .05f,.2f,.4f };
				break;
			case EWayType::path:
				wayColour = { .4f,.3f,.1f };
				break;
			case EWayType::grassland:
				wayColour = { .2f, .5, .3 };
				break;
			case EWayType::sand:
				wayColour = { .8f,.8f,.5f };
				break;
			case EWayType::water:
				wayColour = { .1f,.6f,.8f };
				break;
			case EWayType::defaultt:
				wayColour = { .2f, .2f, .2f };
				break;
			}


			for (const auto& way : wayArr)
			{
				if (way.nodes.size() > 0)
				{
					vertices.push_back(colored_point{ {(float)way.nodes[0].lon,(float)way.nodes[0].lat},wayColour });

					for (const auto& node : way.nodes)
					{
						vertices.push_back(colored_point{ {(float)node.lon,(float)node.lat},wayColour });
						vertices.push_back(colored_point{ {(float)node.lon,(float)node.lat},wayColour });
					}

					vertices.push_back(colored_point{ {(float)way.nodes[way.nodes.size() - 1].lon,(float)way.nodes[way.nodes.size() - 1].lat},wayColour });
				}
			}

			std::cout << "Writing " << sizeof(colored_point) << " * " << vertices.size() << " = " << sizeof(colored_point) * vertices.size() / 1024 << " kB to Buffer " << vbo << "\n";

			vertArr.push_back(vertices);

			auto& asd = vertArr.back();

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(colored_point) * asd.size(), asd.data(), GL_DYNAMIC_DRAW);
		}
	}
};

