import subprocess
import urllib.parse
import urllib.request
import json
import random
import os
from dotenv import load_dotenv

# Load the environment variables from the .env file
load_dotenv()

class ChessBotController:
    def __init__(self, engine_path="Engine/engine.exe"):
        self.engine_process = subprocess.Popen(
            [engine_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )

    def get_lichess_book_move(self, fen: str):
        """Queries the Lichess Masters Opening Explorer API using an auth token."""
        encoded_fen = urllib.parse.quote(fen)
        url = f"https://explorer.lichess.ovh/masters?fen={encoded_fen}"
        
        # Retrieve the token securely
        token = os.getenv("LICHESS_TOKEN")

        try:
            headers = {
                'User-Agent': 'vertixxxx/chess-helper',
                'Accept': 'application/json'
            }
            if token:
                headers['Authorization'] = f"Bearer {token}"

            req = urllib.request.Request(url, headers=headers)
            with urllib.request.urlopen(req, timeout=2.0) as response:
                data = json.loads(response.read().decode())
                moves = data.get("moves", [])
                if moves:
                    top_moves = moves[:3]
                    weights = [m.get("white", 0) + m.get("black", 0) + m.get("draws", 0) for m in top_moves]
                    if sum(weights) > 0:
                        chosen = random.choices(top_moves, weights=weights, k=1)[0]
                        return chosen.get("uci")
        except Exception:
            pass
        return None

    def get_cpp_engine_move(self, fen: str) -> str:
        self.engine_process.stdin.write(f"fen {fen}\n")
        self.engine_process.stdin.flush()

        response = self.engine_process.stdout.readline().strip()
        if response.startswith("bestmove"):
            return response.split()[1]
        return ""

    def get_best_move(self, fen: str) -> str:
        book_move = self.get_lichess_book_move(fen)
        if book_move:
            return book_move
        return self.get_cpp_engine_move(fen)

    def close(self):
        self.engine_process.stdin.write("quit\n")
        self.engine_process.stdin.flush()
        self.engine_process.terminate()