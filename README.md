# Hybrid Chess Engine & GUI

A desktop chess application featuring an interactive graphical user interface built in Python and a custom search and evaluation engine written in C++. The bot uses a hybrid move-selection pipeline that queries the Lichess Opening Explorer API for grandmaster book moves and falls back to a local search engine when out of book.

---

## Features

- Graphical User Interface: Built with Pygame and python-chess, featuring square highlighting, time controls (Bullet, Blitz, Rapid, Unlimited), and dynamic game status overlays.
- C++ Search Engine:
  - Negamax search algorithm with Alpha-Beta pruning.
  - Quiescence search to mitigate the horizon effect during tactical piece exchanges.
  - Mate Distance Pruning (MDP) to prevent search slowdowns during forced mate sequences.
  - MVV-LVA (Most Valuable Victim - Least Valuable Attacker) move ordering to optimize pruning cutoffs.
  - Ray-casting attack detection for rapid check validation.
  - Piece-Square Tables (PST) and piece values for positional and material evaluations.
- Lichess Opening Book Integration: Directly queries the Lichess Masters Opening Explorer REST API with OAuth token authentication to play verified opening lines before engaging the search engine.
- Asynchronous Engine Pipeline: Runs calculation threads in the background to ensure UI responsiveness during engine computation.

---

## Project Structure

chess/
├── Engine/
│   ├── engine.cpp          # C++ chess engine implementation
│   └── engine.exe          # Compiled binary (Windows)
├── images/                 # Piece and board visual assets
├── .env                    # Environment variables (Lichess token)
├── .gitignore              # Ignored files and directories
├── bot_controller.py       # Python-to-C++ subprocess & Lichess API bridge
├── main.py                 # Pygame interface and game loop
└── README.md

---

## Prerequisites

- Python 3.10+
- C++ Compiler supporting C++17 (e.g., MinGW-w64 / GCC, Clang, or MSVC)
- Git

---

## Installation & Setup

1. Clone the repository:
   git clone https://github.com/YOUR_USERNAME/YOUR_REPOSITORY.git
   cd chess

2. Set up a Python virtual environment:
   # Windows
   python -m venv venv
   .\venv\Scripts\activate

   # Linux / macOS
   python3 -m venv venv
   source venv/bin/activate

3. Install dependencies:
   pip install pygame chess python-dotenv

4. Configure environment variables:
   Create a .env file in the root directory:
   LICHESS_TOKEN=your_lichess_personal_access_token_here

5. Compile the C++ Engine:
   # Windows (MinGW / GCC)
   g++ -O3 Engine/engine.cpp -o Engine/engine.exe

   # Linux / macOS
   g++ -O3 Engine/engine.cpp -o Engine/engine

---

## Running the Application

Launch the application using Python:

# Windows
python main.py

# Linux / macOS
python3 main.py

1. Select your preferred time control format on the starting screen.
2. The user plays as White; the bot plays as Black.
3. Moves are made by clicking a piece and then clicking the destination square. Pawn promotions default to Queen automatically.

