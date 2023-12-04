#include <vector>
#include <string>
#include <memory>
#include <variant>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <sstream>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; }; // (1)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;  // (2)

struct StringWithTree;

struct StringWithList;

struct FormatEntry;

class Tree {
private:
	std::unique_ptr<std::variant<StringWithTree, StringWithList>> const value;
public:
	Tree(Tree const& other);
	Tree(StringWithTree const& value);
	Tree(StringWithList const& value);
	std::vector<FormatEntry> to_list() const;
	std::string print() const;
};

enum class Player { Player1 = 1, Player2 = 2 };

enum class Phase { VeryNew, New, Undecided, Decided };

class State {
private:
	std::variant<std::monostate, Player> winning_player;
	std::variant<std::monostate, Tree> winning_description;
	std::variant<std::monostate, uintmax_t> const parent_state_index;
public:
	Player const active_player;
	std::vector<uintmax_t> const spoken_numbers;
	std::vector<uintmax_t> child_state_indices;
	Phase phase;
	State(Player active_player, std::vector<uintmax_t> const& spoken_numbers, std::variant<std::monostate, uintmax_t> parent_state_index);
	bool valid_move(uintmax_t n) const;
	void win(Player winning_player, Tree const& winning_description);
	void update_winning_player();
};

std::vector<State> states;

struct StringWithTree {
	std::string const string;
	Tree const tree;
};

struct StringWithList {
	std::string const string;
	std::vector<Tree> const list;
};

struct FormatEntry {
	uintmax_t const tabs;
	std::string const string;
};

Tree::Tree(Tree const& other) : value(std::make_unique<std::variant<StringWithTree, StringWithList>>(*other.value)) {}

Tree::Tree(StringWithTree const& value) : value(std::make_unique<std::variant<StringWithTree, StringWithList>>(value)) {}

Tree::Tree(StringWithList const& value) : value(std::make_unique<std::variant<StringWithTree, StringWithList>>(value)) {}

std::vector<FormatEntry> Tree::to_list() const {
	return std::visit(overloaded{
		[](StringWithTree const& value) {
			std::vector<FormatEntry> result;
			result.push_back({0, value.string});
			for (FormatEntry const& entry : value.tree.to_list())
				result.push_back({entry.tabs + 1, entry.string});
			return result;
		},
		[](StringWithList const& value) {
			std::vector<FormatEntry> result;
			result.push_back({0, value.string});
			for (Tree const& tree : value.list) {
				for (FormatEntry const& entry : tree.to_list())
					result.push_back({entry.tabs + 1, entry.string});
			}
			return result;
		}
	}, *(this->value));
}

std::string Tree::print() const{
	bool first = true;
	std::string result;
	for (FormatEntry const& entry : this->to_list()) {
		if (first)
			first = false;
		else
			result += "\n";
		for (uintmax_t i = 0; i < entry.tabs; i++)
			result += "\t";
		result += entry.string;
	}
	return result;
}

uintmax_t highest_n = 20;

bool valid_numbers(uintmax_t a, uintmax_t b) {
	return (a % b == 0) || (b % a == 0);
}

Player other_player(Player p) {
	switch(p) {
		case Player::Player1:
			return Player::Player2;
		case Player::Player2:
			return Player::Player1;
	}
}

State::State(Player active_player, std::vector<uintmax_t> const& spoken_numbers, std::variant<std::monostate, uintmax_t> parent_state_index) : active_player(active_player), spoken_numbers(spoken_numbers), parent_state_index(parent_state_index), phase(Phase::VeryNew) {}
bool State::valid_move(uintmax_t n) const {
	if (this->spoken_numbers.empty())
		return n % 2 == 0;
	if (std::find(this->spoken_numbers.begin(), this->spoken_numbers.end(), n) != this->spoken_numbers.end())
		return false;
	return valid_numbers(n, this->spoken_numbers.back());
}
void State::win(Player winning_player, Tree const& winning_description) {
	this->winning_player = winning_player;
	this->winning_description.emplace<Tree>(winning_description);
	this->phase = Phase::Decided;
	std::visit(overloaded{
		[](uintmax_t parent_state_index) {
			states[parent_state_index].update_winning_player();
		},
		[&](std::monostate) {
			std::cout << "(" << static_cast<uintmax_t>(std::get<Player>(this->winning_player)) << ") wins!\n";
			std::cout << std::get<Tree>(this->winning_description).print() << "\n";
			std::exit(0);
		}
	}, this->parent_state_index);
}
void State::update_winning_player() {
	auto is_preferable = [&](uintmax_t child_state_index) {
		return states[child_state_index].phase == Phase::Decided and std::get<Player>(states[child_state_index].winning_player) == this->active_player;
	};
	std::vector<uintmax_t> preferable_child_state_indices;
	for (uintmax_t child_state_index : this->child_state_indices) {
		if (is_preferable(child_state_index))
			preferable_child_state_indices.push_back(child_state_index);
	}
	if (!preferable_child_state_indices.empty()) {
		uintmax_t preferable_child_state_index = preferable_child_state_indices[0];
		std::stringstream string;
		string << "do (" << static_cast<uintmax_t>(this->active_player) << " does \"" << states[preferable_child_state_index].spoken_numbers.back() << "\"), then";
		this->win(this->active_player, Tree(StringWithTree(string.str(), std::get<Tree>(states[preferable_child_state_index].winning_description))));
	}
	auto is_undesirable = [&](uintmax_t child_state_index) {
		return states[child_state_index].phase == Phase::Decided and std::get<Player>(states[child_state_index].winning_player) == other_player(this->active_player);
	};
	bool all_child_states_undesirable = true;
	for (uintmax_t child_state_index : this->child_state_indices) {
		if (!is_undesirable(child_state_index))
			all_child_states_undesirable = false;
	}
	if (all_child_states_undesirable) {
		std::vector<Tree> child_descriptions;
		for (uintmax_t child_state_index : this->child_state_indices) {
			std::stringstream string_inner;
			string_inner << "if (" << static_cast<uintmax_t>(this->active_player) << " does \"" << states[child_state_index].spoken_numbers.back() << "\"), then";
			child_descriptions.push_back(Tree(StringWithTree(string_inner.str(), std::get<Tree>(states[child_state_index].winning_description))));
		}
		std::stringstream string_outer;
		string_outer << "(" << static_cast<uintmax_t>(this->active_player) << ") all impossible:";
		this->win(other_player(this->active_player), Tree(StringWithList(string_outer.str(), child_descriptions)));
	}
}

int main() {
	states.push_back(State(Player::Player1, {}, std::monostate{}));
	states[0].phase = Phase::New;
	while (true) {
		std::cout << states.size() << "\n";
		uintmax_t first_num_states = states.size();
		for (uintmax_t state_index = 0; state_index < first_num_states; state_index++) {
			if (states[state_index].phase == Phase::New) {
				bool found_valid_child_state = false;
				for (uintmax_t n = 1; n <= highest_n; n++) {
					if (states[state_index].valid_move(n)) {
						found_valid_child_state = true;
						std::vector<uintmax_t> new_spoken_numbers = states[state_index].spoken_numbers;
						new_spoken_numbers.push_back(n);
						states.push_back(State(other_player(states[state_index].active_player), new_spoken_numbers, state_index));
						uintmax_t new_state_index = states.size() - 1;
						states[state_index].child_state_indices.push_back(new_state_index);
					}
				}
				states[state_index].phase = Phase::Undecided;
				if (!found_valid_child_state)
					states[state_index].update_winning_player();
			}
		}
		uintmax_t second_num_states = states.size();
		for (uintmax_t state_index = 0; state_index < second_num_states; state_index++) {
			if (states[state_index].phase == Phase::VeryNew)
				states[state_index].phase = Phase::New;
		}
	}
}
