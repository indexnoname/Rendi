// Bro i need to clean all this shit (modules)


#include <dpp/dpp.h>

#include <cstdlib>
#include <stdexcept>

#include <memory>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <ctime>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>


using json = nlohmann::json;

namespace fs = std::filesystem;



static const size_t MAX_CACHE_SIZE = 512 * 1024 * 1024;

static const int MIN_HEIGHT = 1080;
static const int MIN_WIDTH = 1920;










static json project_list;

const std::string LIST_PATH = "ImageRenders/list.json";

const std::string PROJECT_PATH = "ImageRenders/";




struct CachedImage {
	std::string name;
	int width, height, channels;

	std::vector<unsigned char> raw;

	std::vector<unsigned char> png;
};


void set_pixel(CachedImage& img, int x, int y,
	unsigned char r, unsigned char g,
	unsigned char b) {
	size_t idx = (y * img.width + x) * 3;
	img.raw[idx + 0] = r;
	img.raw[idx + 1] = g;
	img.raw[idx + 2] = b;
}










void load_project_list() {
	std::ifstream in(LIST_PATH);
	if (in) in >> project_list;
	else project_list = json::array();
}



void add_project(const json& project) {
	project_list.insert(project_list.begin(), project);
	std::ofstream out(LIST_PATH);
	out << project_list.dump(4);
}
/*
void load_canvas_list() {
	std::ifstream in(CANVAS_PATH);
	if (in) in >> project_list;
	else project_list = json::array();
}



void add_project(const json& project) {
	project_list.insert(project_list.begin(), project);
	std::ofstream out(LIST_PATH);
	out << project_list.dump(4);
}
*/

const std::string BOT_TOKEN = []() {
	char* buffer = nullptr;
	size_t size = 0;
	if (_dupenv_s(&buffer, &size, "DISCORD_TOKEN") != 0 || buffer == nullptr) {
		throw std::runtime_error("Environment variable DISCORD_TOKEN is not set or could not be retrieved.");
	}
	std::string token(buffer);
	free(buffer);
	return token;
	}();



int main() {
	// Setup before initialisation of the bot

	load_project_list();

	// Initialize the bot with the token and intents

	dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content | dpp::i_guild_messages);

	bot.on_log(dpp::utility::cout_logger());





	bot.on_ready([&bot](const dpp::ready_t& event) {
		bot.log(dpp::ll_info, "Bot logged in as " + bot.me.username);
		if (dpp::run_once<struct register_bot_commands>()) {





			const dpp::slashcommand ping = dpp::slashcommand(
				"ping",
				"Check if bot is alive",
				bot.me.id
			);



			const dpp::command_option project_create = dpp::command_option(
				dpp::co_sub_command,
				"create",
				"create a project to solve for humans later on, still in development only 1 shader"
			)
				.add_option(dpp::command_option(dpp::co_string, "name", "Project name", true))
				.add_option(dpp::command_option(dpp::co_attachment, "shader", "Initial render", true))
				.add_option(dpp::command_option(dpp::co_integer, "width", "Width of the end image", true))
				.add_option(dpp::command_option(dpp::co_integer, "height", "Height of the end image", true));





			const dpp::command_option project_list = dpp::command_option(
				dpp::co_sub_command,
				"list",
				"list all of the projects that are currently active"
			);





			const dpp::command_option project_check = dpp::command_option(
				dpp::co_sub_command,
				"check",
				"check the project in the progress. will output an image"
			)
				.add_option(dpp::command_option(dpp::co_string, "name", "Project name", true));





			const dpp::slashcommand project = dpp::slashcommand("project", "Project-related commands", bot.me.id)
				.add_option(project_create)
				.add_option(project_list)
				.add_option(project_check);











			const dpp::command_option pixel_submit = dpp::command_option(
				dpp::co_sub_command,
				"submit",
				"submit pixel as an answer for the image"
			)
				.add_option(dpp::command_option(dpp::co_string, "project", "project name", true))
				.add_option(dpp::command_option(dpp::co_integer, "x", "x coordinate", true))
				.add_option(dpp::command_option(dpp::co_integer, "y", "y coordinate", true))
				.add_option(dpp::command_option(dpp::co_integer, "color", "color of the pixel", true));



			const dpp::command_option pixel_list = dpp::command_option(
				dpp::co_sub_command,
				"list",
				"check available pixels for the project"
			)
				.add_option(dpp::command_option(dpp::co_string, "project", "project name"));

			// maybe add pixel assign

			const dpp::command_option pixel_check = dpp::command_option(
				dpp::co_sub_command,
				"check",
				"check pixel information"
			)
				.add_option(dpp::command_option(dpp::co_string, "project", "project name", true))
				.add_option(dpp::command_option(dpp::co_integer, "x", "x coordinate", true))
				.add_option(dpp::command_option(dpp::co_integer, "y", "y coordinate", true));



			const dpp::slashcommand pixel = dpp::slashcommand("pixel", "Pixel-related commands", bot.me.id)
				.add_option(pixel_submit)
				.add_option(pixel_list)
				.add_option(pixel_check);



			const dpp::slashcommand request = dpp::slashcommand(
				"request",
				"request",
				bot.me.id
			);





			const dpp::command_option canvas_draw = dpp::command_option(
				dpp::co_sub_command,
				"draw",
				"submit pixel as an answer for the image"
			)
				.add_option(dpp::command_option(dpp::co_string, "canvas", "canvas name", true))
				.add_option(dpp::command_option(dpp::co_integer, "x", "x coordinate", true))
				.add_option(dpp::command_option(dpp::co_integer, "y", "y coordinate", true))
				.add_option(dpp::command_option(dpp::co_integer, "color", "color of the pixel", true));



			const dpp::command_option canvas_list = dpp::command_option(
				dpp::co_sub_command,
				"list",
				"list all of the canvases that are currently active"
			);



			const dpp::command_option canvas_check = dpp::command_option(
				dpp::co_sub_command,
				"check",
				"check the canvas clean every time. will output an image"
			)
				.add_option(dpp::command_option(dpp::co_string, "name", "Canvas name", true));



			const dpp::slashcommand canvas = dpp::slashcommand("canvas", "Canvas-related commands", bot.me.id)
				.add_option(canvas_draw)
				.add_option(canvas_list)
				.add_option(canvas_check);



			bot.global_command_create(ping);



			bot.global_command_create(project);



			bot.global_command_create(pixel);



			bot.global_command_create(request);
		}
		});





	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {

		dpp::command_interaction cmd_data = event.command.get_command_interaction();


		if (event.command.get_command_name() == "ping") {
			event.reply("bot is active");
			bot.log(dpp::ll_info, "ping Command executed");
		}





		else if (event.command.get_command_name() == "project") {

			auto subcommand = cmd_data.options[0];

			if (subcommand.name == "create") {

				const std::string project_name = std::get<std::string>(event.get_parameter("name"));
				const dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("shader"));
				const int width = std::get<int64_t>(event.get_parameter("width"));
				const int height = std::get<int64_t>(event.get_parameter("height"));


				if (project_name.empty()) {
					event.reply("Missing or invalid project name");
					return;
				}

				if (project_name.find("..") != std::string::npos ||
					project_name.find('/') != std::string::npos ||
					project_name.find('\\') != std::string::npos) {
					event.reply("Invalid project name");
					return;
				}





				fs::create_directories("ImageRenders/" + project_name + "/shaders");

				std::string path = "ImageRenders/" + project_name + "/blank.png";

				// Create metadata file




				json meta;
				meta["author_id"] = event.command.usr.id;
				meta["author_name"] = event.command.usr.format_username();
				meta["created"] = std::time(nullptr);
				meta["shaders"] = json::array();
				meta["status"] = "active";

				json project;
				project["project_name"] = project_name;
				project["metadata"] = meta;


				add_project(project);


				if (width < MIN_WIDTH || height < MIN_HEIGHT) {
					event.reply("Width and height must be at least " + std::to_string(MIN_WIDTH) + "x" + std::to_string(MIN_HEIGHT));
					return;
				}

				std::vector<unsigned char> img(width * height * 3, 0);

				stbi_write_png(path.c_str(), width, height, 3, img.data(), width * 3);



				const auto& resolved = event.command.resolved;
				auto it = resolved.attachments.find(file_id);
				if (it == resolved.attachments.end()) {
					event.reply("Shader attachment not found.");
					return;
				}
				event.reply(dpp::message("Downloading shader...").set_flags(dpp::m_ephemeral));
				bot.request(it->second.url, dpp::http_method::m_get, [event, project_name](const dpp::http_request_completion_t& res) {
					if (res.status != 200) {
						event.edit_response("Failed to download shader.");
						return;
					}

					// Save shader file
					std::ofstream shader_file("ImageRenders/" + project_name + "/shaders/initial.glsl", std::ios::binary);
					shader_file.write(res.body.data(), res.body.size());
					shader_file.close();

					event.edit_response("Project created successfully! Shader saved as `initial.glsl`");

					});






			}
			else if (subcommand.name == "list") {


				if (project_list.empty()) {
					event.reply("No projects found.");
					return;
				}

				std::string response = "**Projects:**\n";
				for (const auto& proj : project_list) {
					std::string name = proj.value("project_name", "Unnamed");

					const auto& meta = proj["metadata"];
					std::string author = meta.value("author_id", "unknown");
					std::time_t created = meta.value("created", 0);

					response += "- `" + name + "` (by <@" + author + ">) created <t:"
						+ std::to_string(created) + ":R>\n";
				}


				event.reply(response);
			}



			else if (subcommand.name == "check") {

				const std::string project_name = std::get<std::string>(event.get_parameter("name"));

				if (project_list.empty()) {
					event.reply("No projects found.");
					return;
				}

				std::string response;

				for (const auto& proj : project_list) {
					if (!proj.contains("project_name") || proj["project_name"] != project_name)
						continue;

					const auto& meta = proj["metadata"];
					std::string author = meta.value("author_name", "unknown");
					std::time_t created = meta.value("created", 0);
					std::string status = meta.value("status", "unknown");

					response = "**Project:** " + project_name + "\n";
					response += "Author: " + author + "\n";
					response += "Created: <t:" + std::to_string(created) + ":R>\n";
					response += "Status: " + status + "\n";

					event.reply(response);
					return;
				}
				event.reply("Project `" + project_name + "` not found.");



				bot.log(dpp::ll_info, "project Command executed");
			}
		}



		else if (event.command.get_command_name() == "pixel") {

			auto subcommand = cmd_data.options[0];

			if (subcommand.name == "submit") {
				event.reply("Pixel is submited");
			}
			else if (subcommand.name == "list") {
				event.reply("Pixel list:");
			}
			else if (subcommand.name == "check") {
				event.reply("Pixel info:");
			}

			bot.log(dpp::ll_info, "project Command executed");
		}
		});




	/*
	bot.on_message_create([&bot](const dpp::message_create_t& event) {





		if (event.msg.content == "ping") {
			bot.message_create(dpp::message(event.msg.channel_id, "Pong!"));
		}
		});

	*/
	bot.global_commands_get([&bot](const dpp::confirmation_callback_t& cb) {
		if (cb.is_error()) return;
		for (auto& cmd : std::get<dpp::slashcommand_map>(cb.value)) {
			bot.global_command_delete(cmd.second.id);
		}
		});



	bot.start(dpp::st_wait);
}
/*
plan
1. renderer bot will register an account from discord

2. will send the account code to render the information in hand one by one in the messages

3. will check the error rate (if > 10% or 25% (will choose)) then block the account for a day to prevent bad info

4. will add the pixel to the rendered image and add exp to the account

5. when the image is done, it will send the image to the channel in discord




*/


/*
main commands

pixel

project

canvas

*/




/*
-> Load images
-> Use images from the list and check
-> Change the image pixel
-> Save the image



/load image
-> compare used memory with max memory
-> if max memory is reached, remove the least recently used image
-> add the new image to the cache
-> return the image








*/