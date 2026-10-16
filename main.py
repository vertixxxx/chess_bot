import pygame
import chess
import sys
import os
import threading
import random
import math
from bot_controller import ChessBotController

# --- Configuration & Dimensions ---
BASE_BORDER = 8
BASE_SQ_SIZE = 20
DIMENSION = 8
BASE_SIZE = BASE_BORDER * 2 + (BASE_SQ_SIZE * DIMENSION)  # 180

SCALE = 4

BOARD_SIZE = BASE_SIZE * SCALE       # 720
BORDER_SIZE = BASE_BORDER * SCALE    # 32
SQ_SIZE = BASE_SQ_SIZE * SCALE       # 80
PLAY_AREA = BOARD_SIZE - (BORDER_SIZE * 2)

UI_WIDTH = 250
WINDOW_WIDTH = BOARD_SIZE + UI_WIDTH
WINDOW_HEIGHT = BOARD_SIZE
FPS = 30

IMAGES = {}
BOARD_IMAGE = None

PIECE_MAP = {
    'P': 'wp', 'N': 'wN', 'B': 'wB', 'R': 'wR', 'Q': 'wQ', 'K': 'wK',
    'p': 'bp', 'n': 'bN', 'b': 'bB', 'r': 'bR', 'q': 'bQ', 'k': 'bK'
}

PIECE_VALUES = {
    chess.PAWN: 1,
    chess.KNIGHT: 3,
    chess.BISHOP: 3,
    chess.ROOK: 5,
    chess.QUEEN: 9
}

BOT_COLOR = chess.BLACK
is_bot_thinking = False

def get_engine_executable_path():
    """Detects operating system and returns the binary path."""
    if sys.platform.startswith("win"):
        return os.path.join("Engine", "engine.exe")
    return os.path.join("Engine", "engine")

def load_assets():
    """Loads pixel art and scales using nearest-neighbor."""
    global BOARD_IMAGE
    
    board_filename = "board.png" if os.path.exists("board.png") else "image_0.png"
    if not os.path.exists(board_filename):
        BOARD_IMAGE = pygame.Surface((BOARD_SIZE, BOARD_SIZE))
        BOARD_IMAGE.fill((200, 200, 200))
    else:
        raw_board = pygame.image.load(board_filename).convert()
        BOARD_IMAGE = pygame.transform.scale(raw_board, (BOARD_SIZE, BOARD_SIZE))

    if not os.path.exists("images"):
        print("Error: 'images' folder not found.")
        return

    for symbol, filename in PIECE_MAP.items():
        image_path = os.path.join("images", f"{filename}.png")
        try:
            image = pygame.image.load(image_path).convert_alpha()
            orig_w, orig_h = image.get_size()
            IMAGES[symbol] = pygame.transform.scale(image, (orig_w * SCALE, orig_h * SCALE))
        except Exception as e:
            print(f"Could not load {image_path}. Error: {e}")

def get_square_from_pos(pos, flipped=False):
    x, y = pos
    if x >= BOARD_SIZE:
        return None
        
    adj_x = x - BORDER_SIZE
    adj_y = y - BORDER_SIZE
    
    if 0 <= adj_x < PLAY_AREA and 0 <= adj_y < PLAY_AREA:
        col = int(adj_x // SQ_SIZE)
        row = int(adj_y // SQ_SIZE)
        if 0 <= col < 8 and 0 <= row < 8:
            if flipped:
                return chess.square(7 - col, row)
            return chess.square(col, 7 - row)
    return None

def highlight_squares(screen, board, selected_sq, flipped=False):
    if selected_sq is not None:
        if flipped:
            r = chess.square_rank(selected_sq)
            c = 7 - chess.square_file(selected_sq)
        else:
            r = 7 - chess.square_rank(selected_sq)
            c = chess.square_file(selected_sq)
        
        s = pygame.Surface((SQ_SIZE, SQ_SIZE), pygame.SRCALPHA)
        s.fill((0, 100, 255, 110))
        screen.blit(s, (BORDER_SIZE + c * SQ_SIZE, BORDER_SIZE + r * SQ_SIZE))
        
        for move in board.legal_moves:
            if move.from_square == selected_sq:
                if flipped:
                    end_r = chess.square_rank(move.to_square)
                    end_c = 7 - chess.square_file(move.to_square)
                else:
                    end_r = 7 - chess.square_rank(move.to_square)
                    end_c = chess.square_file(move.to_square)
                
                target_surface = pygame.Surface((SQ_SIZE, SQ_SIZE), pygame.SRCALPHA)
                pygame.draw.circle(target_surface, (255, 215, 0, 150), (SQ_SIZE // 2, SQ_SIZE // 2), SQ_SIZE // 6)
                screen.blit(target_surface, (BORDER_SIZE + end_c * SQ_SIZE, BORDER_SIZE + end_r * SQ_SIZE))

def draw_pieces(screen, board, flipped=False):
    for r in range(DIMENSION):
        for c in range(DIMENSION):
            if flipped:
                square = chess.square(7 - c, r)
            else:
                square = chess.square(c, 7 - r)
            piece = board.piece_at(square)
            if piece and piece.symbol() in IMAGES:
                img = IMAGES[piece.symbol()]
                w, h = img.get_size()
                pos_x = BORDER_SIZE + c * SQ_SIZE + (SQ_SIZE - w) // 2
                pos_y = BORDER_SIZE + r * SQ_SIZE + (SQ_SIZE - h) // 2
                screen.blit(img, (pos_x, pos_y))

def calculate_material(board):
    w_score = 0
    b_score = 0
    for pt, val in PIECE_VALUES.items():
        w_score += len(board.pieces(pt, chess.WHITE)) * val
        b_score += len(board.pieces(pt, chess.BLACK)) * val
    
    diff = w_score - b_score
    if diff > 0:
        return f"+{diff}", ""
    elif diff < 0:
        return "", f"+{abs(diff)}"
    return "", ""

def format_time(seconds):
    if seconds is None:
        return "--:--"
    seconds = max(0, int(seconds))
    mins = seconds // 60
    secs = seconds % 60
    return f"{mins:02d}:{secs:02d}"

def draw_ui(screen, font, large_font, board, white_time, black_time, new_game_hover=False):
    pygame.draw.rect(screen, (40, 44, 52), (BOARD_SIZE, 0, UI_WIDTH, WINDOW_HEIGHT))
    pygame.draw.line(screen, (20, 22, 26), (BOARD_SIZE, 0), (BOARD_SIZE, WINDOW_HEIGHT), 5)

    w_mat, b_mat = calculate_material(board)
    
    w_timer_color = (255, 255, 255) if board.turn == chess.WHITE else (150, 150, 150)
    b_timer_color = (255, 255, 255) if board.turn == chess.BLACK else (150, 150, 150)
    
    if BOT_COLOR == chess.BLACK:
        top_title_str = "Black (Bot)"
        top_time = black_time
        top_timer_color = b_timer_color
        top_mat = b_mat
        
        bottom_title_str = "White (You)"
        bottom_time = white_time
        bottom_timer_color = w_timer_color
        bottom_mat = w_mat
    else:
        top_title_str = "White (Bot)"
        top_time = white_time
        top_timer_color = w_timer_color
        top_mat = w_mat
        
        bottom_title_str = "Black (You)"
        bottom_time = black_time
        bottom_timer_color = b_timer_color
        bottom_mat = b_mat

    # --- Opponent Info (Top) ---
    top_title = font.render(top_title_str, True, (200, 200, 200))
    top_time_txt = large_font.render(format_time(top_time), True, top_timer_color)
    top_mat_txt = font.render(top_mat, True, (180, 180, 180))
    
    screen.blit(top_title, (BOARD_SIZE + 20, 30))
    screen.blit(top_time_txt, (BOARD_SIZE + 20, 60))
    screen.blit(top_mat_txt, (BOARD_SIZE + 20, 110))
    
    if is_bot_thinking and board.turn == BOT_COLOR:
        thinking_txt = font.render("Thinking...", True, (255, 215, 0))
        screen.blit(thinking_txt, (BOARD_SIZE + 20, 150))

    # --- New Game Button (Middle) ---
    ng_rect = pygame.Rect(BOARD_SIZE + 25, 335, 200, 48)
    ng_color = (90, 155, 215) if new_game_hover else (60, 110, 160)
    pygame.draw.rect(screen, ng_color, ng_rect, border_radius=8)
    pygame.draw.rect(screen, (200, 200, 200), ng_rect, width=1, border_radius=8)
    ng_text = font.render("New Game", True, (255, 255, 255))
    screen.blit(ng_text, (ng_rect.centerx - ng_text.get_width() // 2, ng_rect.centery - ng_text.get_height() // 2))

    # --- Player Info (Bottom) ---
    bottom_title = font.render(bottom_title_str, True, (200, 200, 200))
    bottom_time_txt = large_font.render(format_time(bottom_time), True, bottom_timer_color)
    bottom_mat_txt = font.render(bottom_mat, True, (180, 180, 180))
    
    screen.blit(bottom_title, (BOARD_SIZE + 20, WINDOW_HEIGHT - 140))
    screen.blit(bottom_time_txt, (BOARD_SIZE + 20, WINDOW_HEIGHT - 110))
    screen.blit(bottom_mat_txt, (BOARD_SIZE + 20, WINDOW_HEIGHT - 60))

    return ng_rect

def time_selection_screen(screen, font, large_font):
    options = [
        {"label": "1 Minute Bullet", "time": 60},
        {"label": "3 Minute Blitz", "time": 180},
        {"label": "10 Minute Rapid", "time": 600},
        {"label": "Unlimited", "time": None}
    ]
    
    clock = pygame.time.Clock()
    
    while True:
        screen.fill((30, 34, 40))
        title = large_font.render("Select Time Format", True, (255, 255, 255))
        screen.blit(title, (WINDOW_WIDTH // 2 - title.get_width() // 2, 100))
        
        mouse_pos = pygame.mouse.get_pos()
        buttons = []
        for i, opt in enumerate(options):
            rect = pygame.Rect(WINDOW_WIDTH // 2 - 160, 200 + i * 80, 320, 60)
            is_hover = rect.collidepoint(mouse_pos)
            btn_color = (90, 155, 215) if is_hover else (70, 130, 180)
            pygame.draw.rect(screen, btn_color, rect, border_radius=10)
            pygame.draw.rect(screen, (255, 255, 255), rect, width=2, border_radius=10)
            text = font.render(opt["label"], True, (255, 255, 255))
            screen.blit(text, (rect.centerx - text.get_width() // 2, rect.centery - text.get_height() // 2))
            buttons.append((rect, opt["time"]))
            
        pygame.display.flip()
        
        for e in pygame.event.get():
            if e.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            elif e.type == pygame.MOUSEBUTTONDOWN and e.button == 1:
                for rect, time_limit in buttons:
                    if rect.collidepoint(e.pos):
                        return time_limit
        clock.tick(FPS)

def color_selection_screen(screen, font, large_font):
    options = [
        {"label": "Play as White", "color": chess.WHITE, "icon": 'K'},
        {"label": "Play as Black", "color": chess.BLACK, "icon": 'k'},
        {"label": "Random Side", "color": "random", "icon": "rand"}
    ]
    
    clock = pygame.time.Clock()
    
    while True:
        screen.fill((30, 34, 40))
        title = large_font.render("Select Your Color", True, (255, 255, 255))
        screen.blit(title, (WINDOW_WIDTH // 2 - title.get_width() // 2, 100))
        
        mouse_pos = pygame.mouse.get_pos()
        buttons = []
        for i, opt in enumerate(options):
            rect = pygame.Rect(WINDOW_WIDTH // 2 - 160, 215 + i * 90, 320, 68)
            is_hover = rect.collidepoint(mouse_pos)
            btn_color = (90, 155, 215) if is_hover else (70, 130, 180)
            pygame.draw.rect(screen, btn_color, rect, border_radius=10)
            pygame.draw.rect(screen, (255, 255, 255), rect, width=2, border_radius=10)
            
            icon_drawn = False
            icon_key = opt["icon"]
            if icon_key in IMAGES and IMAGES[icon_key]:
                try:
                    icon_surf = pygame.transform.scale(IMAGES[icon_key], (44, 44))
                    screen.blit(icon_surf, (rect.left + 20, rect.centery - 22))
                    icon_drawn = True
                except Exception:
                    pass
            elif icon_key == "rand":
                center = (rect.left + 42, rect.centery)
                radius = 18
                pygame.draw.circle(screen, (240, 240, 240), center, radius)
                points = [center]
                for angle in range(-90, 91, 10):
                    rad = math.radians(angle)
                    points.append((center[0] + radius * math.cos(rad), center[1] + radius * math.sin(rad)))
                pygame.draw.polygon(screen, (40, 44, 52), points)
                pygame.draw.circle(screen, (200, 200, 200), center, radius, width=2)
                icon_drawn = True

            text = font.render(opt["label"], True, (255, 255, 255))
            if icon_drawn:
                text_x = rect.left + 70 + (rect.width - 70 - text.get_width()) // 2
            else:
                text_x = rect.centerx - text.get_width() // 2
            screen.blit(text, (text_x, rect.centery - text.get_height() // 2))
            
            buttons.append((rect, opt["color"]))
            
        pygame.display.flip()
        
        for e in pygame.event.get():
            if e.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            elif e.type == pygame.MOUSEBUTTONDOWN and e.button == 1:
                for rect, choice in buttons:
                    if rect.collidepoint(e.pos):
                        if choice == "random":
                            return random.choice([chess.WHITE, chess.BLACK])
                        return choice
        clock.tick(FPS)

def draw_game_over(screen, font, sub_font, message):
    overlay = pygame.Surface((BOARD_SIZE, BOARD_SIZE), pygame.SRCALPHA)
    overlay.fill((0, 0, 0, 180))
    screen.blit(overlay, (0, 0))
    
    text = font.render(message, True, (255, 255, 255))
    sub_text = sub_font.render("Press 'R' or click New Game to play again", True, (210, 210, 210))
    
    text_rect = text.get_rect(center=(BOARD_SIZE // 2, BOARD_SIZE // 2 - 18))
    sub_rect = sub_text.get_rect(center=(BOARD_SIZE // 2, BOARD_SIZE // 2 + 28))
    
    box_w = max(text.get_width(), sub_text.get_width()) + 50
    box_h = text.get_height() + sub_text.get_height() + 50
    box_rect = pygame.Rect(BOARD_SIZE // 2 - box_w // 2, BOARD_SIZE // 2 - box_h // 2, box_w, box_h)
    pygame.draw.rect(screen, (40, 44, 52), box_rect, border_radius=10)
    pygame.draw.rect(screen, (255, 255, 255), box_rect, width=2, border_radius=10)
    
    screen.blit(text, text_rect)
    screen.blit(sub_text, sub_rect)

def check_game_status(board):
    """Checks game conclusion conditions."""
    if board.is_checkmate():
        winner = "White" if board.turn == chess.BLACK else "Black"
        return f"Checkmate! {winner} wins!"
    elif board.is_stalemate():
        return "Draw by Stalemate"
    elif board.is_insufficient_material():
        return "Draw: Insufficient Material"
    return ""

def bot_worker(bot, fen, callback):
    """Worker running in a background thread to calculate the bot's move."""
    move_uci = bot.get_best_move(fen)
    callback(move_uci)

def main():
    global is_bot_thinking, BOT_COLOR
    pygame.init()
    pygame.font.init()
    
    ui_font = pygame.font.SysFont("Arial", 28, bold=True)
    time_font = pygame.font.SysFont("Arial", 48, bold=True)
    go_font = pygame.font.SysFont("Arial", 42, bold=True)
    sub_font = pygame.font.SysFont("Arial", 22)
    
    screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
    pygame.display.set_caption("Chess vs C++ Engine / Lichess Book")
    
    load_assets()
    
    # Initialize C++ engine controller
    engine_bin = get_engine_executable_path()
    bot = ChessBotController(engine_bin)
    
    try:
        while True:
            time_limit = time_selection_screen(screen, ui_font, time_font)
            player_color = color_selection_screen(screen, ui_font, time_font)
            
            BOT_COLOR = chess.BLACK if player_color == chess.WHITE else chess.WHITE
            flipped = (player_color == chess.BLACK)
            
            board = chess.Board()
            
            white_time = time_limit
            black_time = time_limit
            has_timer = time_limit is not None
            
            selected_sq = None
            clicks = []
            game_over_msg = ""
            is_bot_thinking = False
            
            clock = pygame.time.Clock()
            restart_game = False

            def on_bot_move_computed(move_uci):
                global is_bot_thinking
                nonlocal game_over_msg
                moved = False
                
                if move_uci and move_uci != "0000":
                    try:
                        move = chess.Move.from_uci(move_uci)
                        # Auto-promote to Queen if engine omitted promotion suffix
                        if move not in board.legal_moves:
                            promo_move = chess.Move(move.from_square, move.to_square, promotion=chess.QUEEN)
                            if promo_move in board.legal_moves:
                                move = promo_move

                        if move in board.legal_moves:
                            board.push(move)
                            moved = True
                            status = check_game_status(board)
                            if status:
                                game_over_msg = status
                    except ValueError:
                        pass
                        
                # Failsafe if engine failed
                if not moved and not board.is_game_over() and not game_over_msg:
                    legal_moves = list(board.legal_moves)
                    if legal_moves:
                        board.push(legal_moves[0])
                        print(f"Warning: Engine returned invalid move '{move_uci}'.")
                        status = check_game_status(board)
                        if status:
                            game_over_msg = status
                            
                is_bot_thinking = False

            while not restart_game:
                dt = clock.tick(FPS) / 1000.0 
                
                # --- Handle Timers ---
                if not board.is_game_over() and not game_over_msg and has_timer:
                    if board.turn == chess.WHITE:
                        white_time -= dt
                        if white_time <= 0:
                            white_time = 0
                            game_over_msg = "Black wins on time!"
                    else:
                        black_time -= dt
                        if black_time <= 0:
                            black_time = 0
                            game_over_msg = "White wins on time!"

                # --- Trigger Bot Move ---
                if board.turn == BOT_COLOR and not board.is_game_over() and not game_over_msg and not is_bot_thinking:
                    is_bot_thinking = True
                    threading.Thread(
                        target=bot_worker, 
                        args=(bot, board.fen(), on_bot_move_computed), 
                        daemon=True
                    ).start()

                # --- Handle Events ---
                m_pos = pygame.mouse.get_pos()
                ng_rect = pygame.Rect(BOARD_SIZE + 25, 335, 200, 48)
                ng_hover = ng_rect.collidepoint(m_pos)

                for e in pygame.event.get():
                    if e.type == pygame.QUIT:
                        return
                    
                    elif e.type == pygame.KEYDOWN:
                        if e.key == pygame.K_r:
                            restart_game = True
                            break
                    
                    elif e.type == pygame.MOUSEBUTTONDOWN and e.button == 1:
                        if ng_rect.collidepoint(e.pos):
                            restart_game = True
                            break
                        
                        # Prevent interaction if game is over or when it is the bot's turn
                        if board.is_game_over() or game_over_msg or board.turn == BOT_COLOR:
                            continue
                        
                        sq = get_square_from_pos(e.pos, flipped=flipped)
                        if sq is None:
                            continue
                        
                        piece = board.piece_at(sq)
                        if len(clicks) == 0:
                            if piece and piece.color == board.turn:
                                selected_sq = sq
                                clicks.append(sq)
                        elif len(clicks) == 1:
                            if selected_sq == sq:
                                selected_sq = None
                                clicks = []
                            elif piece and piece.color == board.turn:
                                selected_sq = sq
                                clicks = [sq]
                            else:
                                clicks.append(sq)
                        
                        if len(clicks) == 2:
                            move = chess.Move(clicks[0], clicks[1])
                            
                            # Auto-promote pawn to Queen
                            selected_piece = board.piece_at(clicks[0])
                            if selected_piece and selected_piece.piece_type == chess.PAWN:
                                if chess.square_rank(clicks[1]) in (0, 7):
                                    move = chess.Move(clicks[0], clicks[1], promotion=chess.QUEEN)
                            
                            if move in board.legal_moves:
                                board.push(move)
                                status = check_game_status(board)
                                if status:
                                    game_over_msg = status
                            
                            selected_sq = None
                            clicks = []
                
                # --- Rendering ---
                screen.fill((0, 0, 0))
                
                screen.blit(BOARD_IMAGE, (0, 0))
                highlight_squares(screen, board, selected_sq, flipped=flipped)
                draw_pieces(screen, board, flipped=flipped)
                
                draw_ui(screen, ui_font, time_font, board, white_time, black_time, new_game_hover=ng_hover)
                
                if game_over_msg:
                    draw_game_over(screen, go_font, sub_font, game_over_msg)

                pygame.display.flip()
    finally:
        bot.close()
        pygame.quit()

if __name__ == "__main__":
    main()