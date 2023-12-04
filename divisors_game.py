def flatten(lst):
    return [inner for outer in lst for inner in outer]

class StringWithTree:
    def __init__(self, s, t):
        self.string = s
        self.tree = t

class StringWithList:
    def __init__(self, s, l):
        self.string = s
        self.list = l

class TreeType:
    StringWithTree = 0
    StringWithList = 1

class Tree:
    def __init__(self, tree_type, value):
        self.tree_type = tree_type
        self.value = value

    def to_list(self):
        if self.tree_type == TreeType.StringWithTree:
            return [(0, self.value.string)] + [(n + 1, s) for (n, s) in self.value.tree.to_list()]
        if self.tree_type == TreeType.StringWithList:
            return [(0, self.value.string)] + flatten([[(n + 1, s) for (n, s) in tree.to_list()] for tree in self.value.list])

    def print(self):
        return "\n".join(["\t"*n + s for (n, s) in self.to_list()])

highest_n = 20

def valid_numbers(a, b):
    return a % b == 0 or b % a == 0

def other_player(p):
    return (p % 2) + 1

class Phase:
    VeryNew = 0
    New = 1
    Undecided = 2
    Decided = 3

class State:
    def __init__(self, active_player, spoken_numbers, parent_state_index):
        self.active_player = active_player
        self.winning_player = None
        self.winning_description = None
        self.spoken_numbers = spoken_numbers
        self.child_state_indices = []
        self.parent_state_index = parent_state_index
        self.phase = Phase.VeryNew

    def valid_move(self, n):
        if self.spoken_numbers == []:
            return n % 2 == 0
        if n in self.spoken_numbers:
            return False
        return valid_numbers(n, self.spoken_numbers[-1])

    def win(self, winning_player, winning_description):
        self.winning_player = winning_player
        self.winning_description = winning_description
        self.phase = Phase.Decided
        if self.parent_state_index != None:
            states[self.parent_state_index].update_winning_player()
        else:
            print(f"({self.winning_player}) wins!")
            print(self.winning_description.print())
            raise Exception("Success")

    def update_winning_player(self):
        def is_preferable(child_state_index):
            child_state = states[child_state_index]
            return child_state.phase == Phase.Decided and child_state.winning_player == self.active_player
        preferable_child_state_indices = list(filter(is_preferable, self.child_state_indices))
        if preferable_child_state_indices != []:
            preferable_child_state_index = preferable_child_state_indices[0]
            preferable_child_state = states[preferable_child_state_index]
            self.win(self.active_player, Tree(TreeType.StringWithTree, StringWithTree(
                f"do ({self.active_player} does \"{preferable_child_state.spoken_numbers[-1]}\"), then",
                preferable_child_state.winning_description
                )))
        def is_undesirable(child_state_index):
            child_state = states[child_state_index]
            return child_state.phase == Phase.Decided and child_state.winning_player == other_player(self.active_player)
        all_child_states_undesirable = all([is_undesirable(child_state_index) for child_state_index in self.child_state_indices])
        if all_child_states_undesirable:
            child_descriptions = [Tree(TreeType.StringWithTree, StringWithTree(
                f"if ({self.active_player} does \"{states[child_state_index].spoken_numbers[-1]}\"), then",
                states[child_state_index].winning_description
                )) for child_state_index in self.child_state_indices]
            self.win(other_player(self.active_player), Tree(TreeType.StringWithList, StringWithList(
                f"({self.active_player}) all impossible:",
                child_descriptions
                )))

states = []
states.append(State(1, [], None))
states[0].phase = Phase.New

while True:
    print(len(states))
    for state_index in range(len(states)):
        state = states[state_index]
        if state.phase == Phase.New:
            found_valid_child_state = False
            for n in range(1, highest_n + 1):
                if state.valid_move(n):
                    found_valid_child_state = True
                    states.append(State(other_player(state.active_player), state.spoken_numbers + [n], state_index))
                    new_state_index = len(states) - 1
                    state.child_state_indices.append(new_state_index)
            state.phase = Phase.Undecided
            if not found_valid_child_state:
                state.update_winning_player()
    for state_index in range(len(states)):
        state = states[state_index]
        if state.phase == Phase.VeryNew:
            state.phase = Phase.New
