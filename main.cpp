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

#define VTX_STRIDE 3
#define NRM_STRIDE 3
#define TXC_STRIDE 2

#define VTX_IDX "v"
#define VNRM_IDX "vn"
#define VTEX_IDX "vt"

#define SRC_VTXA_DECL_START 0
#define SRC_VTXA_DECL_END 1

#define OBJTX_ERRLOG(...) std::cerr<<"! OBJTX ERROR: "<<__VA_ARGS__<<'\n'

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
	{"-py", F_Py_OUTPUT},
	{"-raw", F_RAW_OUTPUT},
	{"-v", F_DMP_VRTX},
	{"-n", F_DMP_NRML},
	{"-vt", F_DMP_VTEX}
};

std::unordered_map<uint8_t, std::vector<std::string>> src_imps = {
	{ F_C_OUTPUT, { "float vertices[] = {", "};" } },
	{ F_Py_OUTPUT, { "vertices = [", "]" } },
}; 

std::unordered_map<uint8_t, uint8_t> disablers = {
	{ F_C_OUTPUT, F_Py_OUTPUT },
	{ F_Py_OUTPUT, F_C_OUTPUT }
};

std::unordered_map<std::string, std::unordered_map<std::string, std::vector<size_t>>> named_obj_faces;
std::unordered_map<std::string, color> obj_colors; 

static inline size_t get_vtx_id(const std::string &str){
	return str.empty() ? SIZE_MAX : std::stoll(str) - 1;
}

std::string make_txt_dump(const std::vector<std::string> &data, bool last){
	std::string res = "";
	for(const auto &d : data){
		res = res.empty() ? d : res + ", " + d;
	}
	
	return last ? res : res + ",";
}

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

void parse_indices(const std::string &last_obj, const std::string &token){
	if(token.empty()){
		named_obj_faces[last_obj][VTX_IDX].push_back(SIZE_MAX);
		named_obj_faces[last_obj][VTEX_IDX].push_back(SIZE_MAX);
		named_obj_faces[last_obj][VNRM_IDX].push_back(SIZE_MAX);
	}
	
	std::string indices[] = { VTX_IDX, VTEX_IDX, VNRM_IDX };
	uint8_t id = 0;
	std::string current_idx = "";
	
	for(size_t i = 0; i < token.length(); i++){
		switch(token[i]){
			case '/':
				named_obj_faces[last_obj][indices[id]].push_back(get_vtx_id(current_idx));
				current_idx.clear();
				id = id < 2 ? id + 1 : 0;
			break;
			default:
				current_idx.push_back(token[i]);
			break;
		}
	}
	for(; id < 3; id++){
		named_obj_faces[last_obj][indices[id]].push_back(get_vtx_id(current_idx));
		current_idx.clear();
	}
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
			named_obj_faces.emplace(last_obj, std::unordered_map<std::string, std::vector<size_t>>{ {VTX_IDX, {}}, {VTEX_IDX, {}}, {VNRM_IDX, {}} });
			continue;
		}
		if(token_line[0] == 'o' && named_obj_faces.find(token_line.substr(2)) != named_obj_faces.end()){
			last_obj = token_line.substr(2);
			continue;
		}
		if(token_line.substr(0, type.length()) != type){
			continue; 
		}
		
		size_t prev_tok_pos = token_line.find(' ');
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
			while((tok_pos = token_line.find(' ', tok_pos)) != std::string::npos){
				if(tok_pos > prev_tok_pos){
					std::string indices = token_line.substr(prev_tok_pos + 1, tok_pos - prev_tok_pos - 1);
					parse_indices(last_obj, indices);
					tok_counter++;
					prev_tok_pos = tok_pos;
				}
				tok_pos++;
			}
			std::string indices = token_line.substr(prev_tok_pos + 1, tok_pos - prev_tok_pos - 1);
			parse_indices(last_obj, indices);
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
			
			// for all dump flags enabled the order of values FOR EACH VERTEX is as follows:
			// vertex coordinates (x,y,z), normal coordinates (x,y,z), texture coordinates (x,y), color(r,g,b)
			
			_o_objd << src_imps[flags & (F_C_OUTPUT | F_Py_OUTPUT)][SRC_VTXA_DECL_START] << '\n';
			for(auto &o : named_obj_faces){
				std::cout<<"dump object \'"<<o.first<<"\' vertices("<<o.second[VTX_IDX].size()<<'/'<<o.second[VTEX_IDX].size()<<'/'<<o.second[VNRM_IDX].size()<<')'<<std::endl;
				for(size_t i = 0; i < o.second[VTX_IDX].size(); i++){
					_o_objd << '\t';
					std::vector<std::string> vtx_data;
					if(flags & F_DMP_VRTX){
						if(o.second[VTX_IDX][i] * VTX_STRIDE + 2 < vertices_coords.size()){
							vtx_data.push_back(vertices_coords[o.second[VTX_IDX][i] * VTX_STRIDE]);
							vtx_data.push_back(vertices_coords[o.second[VTX_IDX][i] * VTX_STRIDE + 1]); 
							vtx_data.push_back(vertices_coords[o.second[VTX_IDX][i] * VTX_STRIDE + 2]);
						}
						else{
							OBJTX_ERRLOG("Vertex coordinates were not found for object "<<o.first<<", filling data with 0");
							vtx_data.push_back("0.0");
							vtx_data.push_back("0.0");
							vtx_data.push_back("0.0");
						}
						total_triangles_out++;
					}
					if(flags & F_DMP_NRML){
						if(o.second[VNRM_IDX][i] * NRM_STRIDE + 2 < vertices_nrmcoords.size()){
							vtx_data.push_back(vertices_nrmcoords[o.second[VNRM_IDX][i] * NRM_STRIDE]);
							vtx_data.push_back(vertices_nrmcoords[o.second[VNRM_IDX][i] * NRM_STRIDE + 1]); 
							vtx_data.push_back(vertices_nrmcoords[o.second[VNRM_IDX][i] * NRM_STRIDE + 2]);
						}
						else{
							OBJTX_ERRLOG("Vertex normals were not found for object "<<o.first<<", filling data with 0");
							vtx_data.push_back("0.0");
							vtx_data.push_back("0.0");
							vtx_data.push_back("0.0");
						}
					}
					if(flags & F_DMP_VTEX){
						if(o.second[VTEX_IDX][i] * TXC_STRIDE + 1 < vertices_texcoords.size()){
							vtx_data.push_back(vertices_texcoords[o.second[VTEX_IDX][i] * TXC_STRIDE]);
							vtx_data.push_back(vertices_texcoords[o.second[VTEX_IDX][i] * TXC_STRIDE + 1]);
						}
						else{
							OBJTX_ERRLOG("Vertex textures were not found for object "<<o.first<<", filling data with 0");
							vtx_data.push_back("0.0");
							vtx_data.push_back("0.0");
						}
					}
					
					if(obj_colors.find(o.first) != obj_colors.end()){
						vtx_data.push_back(std::to_string((float)((float)obj_colors[o.first].r / 255.0f)));
						vtx_data.push_back(std::to_string((float)((float)obj_colors[o.first].g / 255.0f)));
						vtx_data.push_back(std::to_string((float)((float)obj_colors[o.first].b / 255.0f)));
					}
					_o_objd << make_txt_dump(vtx_data, i + 1 >= o.second[VTX_IDX].size()) << '\n';
				}
			}
			_o_objd << src_imps[flags & (F_C_OUTPUT | F_Py_OUTPUT)][SRC_VTXA_DECL_END] << '\n';
			_o_objd.close();
		}
		else{
			std::ofstream _o_objd;
			_o_objd.open(save.c_str(), std::fstream::binary);
			
			// for all dump flags enabled the order of values FOR EACH VERTEX is as follows:
			// vertex coordinates (x,y,z) = 12 bytes(3 floats)
			// normal coordinates (x,y,z) = 12 bytes(3 floats)
			// texture coordinates (x,y) = 8 bytes(2 floats)
			// color (r,g,b) = 12 bytes(3 floats)
			//	summary = 44 bytes per vertex, assuming, that sizeof(float) == 4 bytes
			
			for(auto &o : named_obj_faces){
				std::cout<<"dump object \'"<<o.first<<"\' vertices("<<o.second[VTX_IDX].size()<<')'<<std::endl;
				for(size_t i = 0; i < o.second[VTX_IDX].size(); i++){
					if(flags & F_DMP_VRTX){
						if(o.second[VTX_IDX][i] * VTX_STRIDE + 2 < vertices_coords.size()){
							for(size_t j = o.second[VTX_IDX][i] * VTX_STRIDE; j < o.second[VTX_IDX][i] * VTX_STRIDE + VTX_STRIDE; j++){
								float value = std::stof(vertices_coords[j]);
								_o_objd.write((char*)&value, sizeof(value));
							}
						}
						else{
							OBJTX_ERRLOG("Vertex coordinates were not found for object "<<o.first<<", filling data with 0");
							float values[3] = {0};
							_o_objd.write((char*)values, sizeof(values));
						}
						total_triangles_out++;
					}
					if(flags & F_DMP_NRML){
						if(o.second[VNRM_IDX][i] * NRM_STRIDE + 2 < vertices_nrmcoords.size()){
							for(size_t j = o.second[VNRM_IDX][i] * NRM_STRIDE; j < o.second[VNRM_IDX][i] * NRM_STRIDE + NRM_STRIDE; j++){
								float value = std::stof(vertices_nrmcoords[j]);
								_o_objd.write((char*)&value, sizeof(value));
							}
						}
						else{
							OBJTX_ERRLOG("Vertex normals were not found for object "<<o.first<<", filling data with 0");
							float values[3] = {0};
							_o_objd.write((char*)values, sizeof(values));
						}
					}
					if(flags & F_DMP_VTEX){
						if(o.second[VTEX_IDX][i] * TXC_STRIDE + 1 < vertices_texcoords.size()){
							for(size_t j = o.second[VTEX_IDX][i] * TXC_STRIDE; j < o.second[VTEX_IDX][i] * TXC_STRIDE + TXC_STRIDE; j++){
								float value = std::stof(vertices_texcoords[j]);
								_o_objd.write((char*)&value, sizeof(value));
							}
						}
						else{
							OBJTX_ERRLOG("Vertex textures were not found for object "<<o.first<<", filling data with 0");
							float values[2] = {0};
							_o_objd.write((char*)values, sizeof(values));
						}
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

std::string cpp_str_lower(char *str){
	std::string res = "";
	for(; *str != '\0'; str++){
		res.push_back(tolower(*str));
	}
	return res;
}

void mutual_disable(uint8_t &flags, uint8_t set_flag){
	if(disablers.find(set_flag) == disablers.end()){
		return;
	}
	
	flags &= ~disablers[set_flag];
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
		if(cpp_str_lower(argv[i]) == "-cl"){
			obj_colors.emplace(argv[i + 1], color{(uint8_t)std::atoi(argv[i + 2]), (uint8_t)std::atoi(argv[i + 3]), (uint8_t)std::atoi(argv[i + 4])});
			i += 5;
			continue;
		}
		if(cpp_str_lower(argv[i]) == "-f"){
			filepath_argvidx = i + 1;
			i += 2;
			continue;
		}
		if(cpp_str_lower(argv[i]) == "-of"){
			filename_argvidx = i + 1;
			i += 2;
			continue;
		}
		if(map_flags.find(cpp_str_lower(argv[i])) != map_flags.end()){
			flags |= map_flags[argv[i]];
			mutual_disable(flags, map_flags[argv[i]]);
			i++;
			continue;
		}
		std::cout<<"Unknown flag \'"<<argv[i]<<"\' has no effect."<<std::endl;
		i++;
	}
	extractor(argv[filename_argvidx], argv[filepath_argvidx], flags);
	
	return EXIT_SUCCESS;
}
