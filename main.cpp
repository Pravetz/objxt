// This project is distributed under BSD 3-clause license
// see LICENSE for more details
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <unordered_map>
#include "proginfo.h"

enum FLAGS : uint8_t {
	F_C_OUTPUT = (1 << 0),
	F_Py_OUTPUT = (1 << 1),
	F_DMP_VRTX = (1 << 2),
	F_DMP_NRML = (1 << 3),
	F_DMP_VTEX = (1 << 4),
	F_RAW_OUTPUT = (1 << 5)
};

struct color{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

std::unordered_map<std::string, uint8_t> map_flags = {
	{"-c", F_C_OUTPUT},
	{"-C", F_C_OUTPUT},
	{"-py", F_Py_OUTPUT},
	{"-Py", F_Py_OUTPUT},
	{"-raw", F_RAW_OUTPUT},
	{"-v", F_DMP_VRTX},
	{"-V", F_DMP_VRTX},
	{"-n", F_DMP_NRML},
	{"-N", F_DMP_NRML},
	{"-vt", F_DMP_VTEX},
	{"-VT", F_DMP_VTEX}
};

std::unordered_map<std::string, std::vector<size_t>> named_obj_faces; 
std::unordered_map<std::string, color> obj_colors; 

size_t count_obj_count(std::ifstream &stream){
	stream.seekg(0);
	size_t count = 0;
	std::string token_line = "";
	while(std::getline(stream, token_line)){
		if(token_line[0] == 'o'){
			count++;
		}
	}
	
	return count;
}

void extract(const std::string &type, std::ifstream &stream, std::vector<std::string> &save_to){
	if(type != "f "){
		save_to.clear();
	}
	std::string token_line = "";
	stream.clear();
	stream.seekg(std::ios_base::beg);
	std::string last_obj = "";
	while(std::getline(stream, token_line)){
		if(token_line[0] == 'o' && named_obj_faces.find(token_line.substr(2)) == named_obj_faces.end()){
			std::cout<<"o "<<token_line.substr(2)<<std::endl;
			last_obj = token_line.substr(2);
			named_obj_faces.emplace(last_obj, std::vector<size_t>());
			continue;
		}
		if(token_line[0] == 'o' && named_obj_faces.find(token_line.substr(2)) != named_obj_faces.end()){
			last_obj = token_line.substr(2);
			continue;
		}
		if(token_line.substr(0, type.length()) != type){
			continue; 
		}
		
		size_t prev_tok_pos = token_line.find_first_of(" /");
		size_t tok_pos = 0;
		if(type != "f "){
			while((tok_pos = token_line.find(' ', tok_pos)) != std::string::npos){
				if(tok_pos > prev_tok_pos){
					save_to.push_back(token_line.substr(prev_tok_pos + 1, tok_pos - prev_tok_pos - 1));
					prev_tok_pos = tok_pos;
				}
				tok_pos++;
			}
			if(prev_tok_pos != std::string::npos){
				save_to.push_back(token_line.substr(prev_tok_pos + 1));
			}
		}
		else{
			size_t tok_counter = 0;
			while((tok_pos = token_line.find_first_of(" /", tok_pos)) != std::string::npos){
				if(tok_pos > prev_tok_pos){
					if(tok_counter % 3 == 0){
						named_obj_faces[last_obj].push_back(std::stoll(token_line.substr(prev_tok_pos + 1, tok_pos - prev_tok_pos - 1)) - 1);
					}
					tok_counter++;
					prev_tok_pos = tok_pos;
				}
				tok_pos++;
			}
		}
	}
}

void extractor(const std::string &savename, const std::string &fpath, const uint8_t flags){
	std::ifstream _i_obj;
	_i_obj.open(fpath.c_str());
	
	if(_i_obj){
		std::string save = savename;
		if(savename == fpath.substr(fpath.find_last_of("/\\") + 1)){
			save += ".txt";
		}
		std::vector<std::string> vertices_coords;
		std::vector<std::string> vertices_texcoords;
		std::vector<std::string> vertices_nrmcoords;
		extract("v ", _i_obj, vertices_coords);
		extract("vt", _i_obj, vertices_texcoords);
		extract("vn", _i_obj, vertices_nrmcoords);
		extract("f ", _i_obj, vertices_coords);
		
		std::cout<<"extracted:\n"<<vertices_coords.size()<<"\tvertex coordinates\n"<<vertices_texcoords.size()<<"\ttexture coordinates\n"<<vertices_nrmcoords.size()<<"\tnormal coordinates"<<std::endl;
		
		
		size_t total_triangles_out = 0;
		if((flags & F_RAW_OUTPUT) == 0){
			std::ofstream _o_objd;
			_o_objd.open(save.c_str());
		
			if(flags & F_C_OUTPUT){
				_o_objd << "float vertices[] = {\n";
			}
			else{
				_o_objd << "vertices = [\n";
			}
			for(auto &o : named_obj_faces){
				std::cout<<"dump object \'"<<o.first<<"\' vertices("<<o.second.size()<<')'<<std::endl;
				for(size_t i = 0; i < o.second.size(); i++){
					_o_objd << '\t';
					if(flags & F_DMP_VRTX){
						_o_objd << vertices_coords[o.second[i] * 3] << ", " << vertices_coords[o.second[i] * 3 + 1] << ", " << vertices_coords[o.second[i] * 3 + 2];
						total_triangles_out++;
					}
					
					if(obj_colors.find(o.first) != obj_colors.end()){
						_o_objd << ", " << (float)((float)obj_colors[o.first].r / 255.0f) << ", " << (float)((float)obj_colors[o.first].g / 255.0f) << ", " << (float)((float)obj_colors[o.first].b / 255.0f);
					}
					_o_objd << ",\n";
				}
			}
			if(flags & F_C_OUTPUT){
				_o_objd << "\n};";
			}
			else{
				_o_objd << "\n]";
			}
			_o_objd.close();
		}
		else{
			std::ofstream _o_objd;
			_o_objd.open(save.c_str(), std::fstream::binary);
			
			for(auto &o : named_obj_faces){
				std::cout<<"dump object \'"<<o.first<<"\' vertices("<<o.second.size()<<')'<<std::endl;
				for(size_t i = 0; i < o.second.size(); i++){
					if(flags & F_DMP_VRTX){
						for(size_t j = o.second[i] * 3; j < o.second[i] * 3 + 3; j++){
							float value = std::stof(vertices_coords[j]);
							_o_objd.write((char*)&value, sizeof(value));
						}
						
						total_triangles_out++;
					}
					
					if(obj_colors.find(o.first) != obj_colors.end()){
						float r = ((float)obj_colors[o.first].r / 255.0f), g = ((float)obj_colors[o.first].g / 255.0f), b = ((float)obj_colors[o.first].b / 255.0f);
						_o_objd.write((char*)&r, sizeof(r));
						_o_objd.write((char*)&g, sizeof(g));
						_o_objd.write((char*)&b, sizeof(b));
					}
				}
			}
			
			_o_objd.close();
		}
		std::cout<<"Successfully extracted "<<total_triangles_out<<" triangles from \'"<<fpath<<"\'"<<std::endl;
	}
	else{
		std::cout<<"Failed to open file in path \'"<<fpath<<"\'"<<std::endl;
	}
}

int main(int argc, char **argv){
	if(argc == 1){
		std::cout<<OBJXT_PROGINFO<<OBJXT_HELP(argv[0])<<std::endl;
		return EXIT_SUCCESS;
	}
	uint8_t flags = 0;
	size_t filepath_argvidx = 1;
	size_t filename_argvidx = 1;
	for(size_t i = 1; i < (size_t)argc; ){
		if(strcmp(argv[i], "-cl") == 0 || strcmp(argv[i], "-CL") == 0){
			obj_colors.emplace(argv[i + 1], color{(uint8_t)std::atoi(argv[i + 2]), (uint8_t)std::atoi(argv[i + 3]), (uint8_t)std::atoi(argv[i + 4])});
			i += 5;
			continue;
		}
		if(strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "-F") == 0){
			filepath_argvidx = i + 1;
			i += 2;
			continue;
		}
		if(strcmp(argv[i], "-of") == 0 || strcmp(argv[i], "-OF") == 0){
			filename_argvidx = i + 1;
			i += 2;
			continue;
		}
		if(map_flags.find(argv[i]) != map_flags.end()){
			flags |= map_flags[argv[i]];
			i++;
			continue;
		}
		std::cout<<"Unknown flag \'"<<argv[i]<<"\' has no effect."<<std::endl;
		i++;
	}
	extractor(argv[filename_argvidx], argv[filepath_argvidx], flags);
	
	return EXIT_SUCCESS;
}
