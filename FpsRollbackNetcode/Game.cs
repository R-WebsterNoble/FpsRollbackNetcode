using GameLogic;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;

namespace FpsRollbackNetcode;

public class Game : Microsoft.Xna.Framework.Game
{
    private readonly GraphicsDeviceManager _graphics;
    private SpriteBatch _spriteBatch;
    
    //Camera
    private Vector3 _bunnyPosition;
    private Vector3 _camPosition;
    private Matrix _projectionMatrix;
    private Matrix _viewMatrix;
    private Matrix _worldMatrix;

    //Geometric info
    private Model _bunnyModel;

    private Vector2 _cameraRotation = new Vector2();
    private MouseState _previousMouseState = new MouseState();

    private const float TICKS_PER_SECOND = 128f;
        
    private const float MAX_ROLLBACK_TIME = 500f;
    private const int PLAYER_COUNT = 10;
        
    private const float ACCELERATION = 0.01f / 1000f;
    private const float MAX_VELOCITY = 0.002f;
        
    private const float SMOOTHING_LERP = 0.0005f;
        
    private const float SMOOTHING_LINEAR = MAX_VELOCITY * 1.45f;
        
    private const float WIGGLE_FREQUENCY = 2000f;
        
    private readonly float _skyboxDiameter = 1000000f;
    // private TimeSpan _frameProcTime;
        
    private bool _controlAll = true;
        
    private GameState _gameState;
        
    private GameStateManager _gameStateManager;
    // private BlSprite _hudBackground;
        
        
    // private Matrix _lastProjectionMatrix;
    // private SpriteFont _font;
        
    private KeyboardState _previousKeyboardState;
    private GameState _realTimeGameState;
    private GameState _delayedGameState;
        
    private ResycSmoothing _resycSmoothing;
    private PlayerState[] _smoothedPlayers;

    private int _delay;
        
    private bool _displayRealTimePlayer = true;
    private bool _displayClientPlayers;
    private bool _displaySmoothedPlayer;

    public Game()
    {
        _graphics = new GraphicsDeviceManager(this);
        Content.RootDirectory = "Content";
    }

    protected override void Initialize()
    {
        base.Initialize();

        //Setup Camera
        _bunnyPosition = Vector3.Zero;
        _camPosition = Vector3.Backward * 7f + Vector3.Up * 2f;
        _projectionMatrix = Matrix.CreatePerspectiveFieldOfView(MathHelper.ToRadians(45f),
            _graphics.GraphicsDevice.Viewport.AspectRatio, 1f, 1000f);
        _viewMatrix = Matrix.CreateLookAt(_camPosition, _bunnyPosition, Vector3.Up);
        _worldMatrix = Matrix.CreateWorld(_bunnyPosition, Vector3.Forward, Vector3.Up);

        // _cameraRotation = new Vector2(0f, MathHelper.ToRadians(-45f));
        _cameraRotation = new Vector2(0f, -0.25f);

        _bunnyModel = Content.Load<Model>("bunny");

        IsFixedTimeStep = false;

        _gameStateManager = new GameStateManager(1000f / TICKS_PER_SECOND, MAX_ROLLBACK_TIME, PLAYER_COUNT, WIGGLE_FREQUENCY);
            
        _resycSmoothing = new ResycSmoothing(_gameStateManager.Latest.Players, SMOOTHING_LERP, SMOOTHING_LINEAR);
            
        //GameState = new GameState() 
        //{
        //	Players = Enumerable.Range(0, 10).Select(i => new PlayerState() { Position = new Vector3((float)Rand.NextDouble()-0.5f, (float)Rand.NextDouble()-0.5f, 0f) }).ToArray()
        //};
            
        _bunnyModel = Content.Load<Model>("bunny");

        for (var i = 1; i < _gameStateManager.PlayerCount; i++)
        {
            var playerRollbackTicks = _gameStateManager.GetDelayedPlayerRollbackTicks(i);
            Console.WriteLine($"player : {i} = {playerRollbackTicks * _gameStateManager.TickDuration}");
        }

    }

    protected override void LoadContent()
    {
        _spriteBatch = new SpriteBatch(GraphicsDevice);
    }

    protected override void UnloadContent()
    {

    }

    protected override void Update(GameTime gameTime)
    {
        var keyboardState = Keyboard.GetState();
        var mouseState = Mouse.GetState();

        if (GamePad.GetState(PlayerIndex.One).Buttons.Back ==
            ButtonState.Pressed || keyboardState.IsKeyDown(Keys.Escape))
            Exit();
        
        var mouseDelta = Point.Zero;

        if (keyboardState.IsKeyDown(Keys.Space) || mouseState.RightButton == ButtonState.Pressed)
        {
            var windowCenter = new Point(Window.ClientBounds.Width / 2, Window.ClientBounds.Height / 2);

            if (_previousKeyboardState.IsKeyDown(Keys.Space) || _previousMouseState.RightButton == ButtonState.Pressed)
            {
                mouseDelta = windowCenter - mouseState.Position;
            }

            Mouse.SetPosition(windowCenter.X, windowCenter.Y);
        }

        const float CAMERA_MOVEMENT_SPEED = 0.005f;
        var movementDelta = (CAMERA_MOVEMENT_SPEED * (float)gameTime.ElapsedGameTime.TotalMilliseconds);

        var camMovement = Vector3.Zero;


        if (keyboardState.IsKeyDown(Keys.Space))
        {
            if (keyboardState.IsKeyDown(Keys.W))
            {
                camMovement += Vector3.Forward * movementDelta;
            }
            if (keyboardState.IsKeyDown(Keys.S))
            {
                camMovement += Vector3.Backward * movementDelta;
            }
            if (keyboardState.IsKeyDown(Keys.D))
            {
                camMovement += Vector3.Right * movementDelta;
            }
            if (keyboardState.IsKeyDown(Keys.A))
            {
                camMovement += Vector3.Left * movementDelta;
            }
            if (keyboardState.IsKeyDown(Keys.LeftShift))
            {
                camMovement += Vector3.Up * movementDelta;
            }
            if (keyboardState.IsKeyDown(Keys.LeftControl))
            {
                camMovement += Vector3.Down * movementDelta;
            }

            if (keyboardState.IsKeyDown(Keys.Up))
            {
                _bunnyPosition += Vector3.Forward * movementDelta;
            }
            if (keyboardState.IsKeyDown(Keys.Down))
            {
                _bunnyPosition += Vector3.Backward * movementDelta;
            }
            if (keyboardState.IsKeyDown(Keys.Right))
            {
                _bunnyPosition += Vector3.Right * movementDelta;
            }
            if (keyboardState.IsKeyDown(Keys.Left))
            {
                _bunnyPosition += Vector3.Left * movementDelta;
            }
            if (keyboardState.IsKeyDown(Keys.PageUp))
            {
                _bunnyPosition += Vector3.Up * movementDelta;
            }
            if (keyboardState.IsKeyDown(Keys.PageDown))
            {
                _bunnyPosition += Vector3.Down * movementDelta;
            }
        }

        if (keyboardState.IsKeyDown(Keys.Space))
        {
            float WrapRadians(float value)
            {
                var times = (float)Math.Floor((value + 360f) / (360f * 2f));

                return value - (times * (360f * 2f));
            }

            const float MOUSE_SENSITIVITY = 0.003f;
            var newXRot = WrapRadians(_cameraRotation.X + mouseDelta.X * MOUSE_SENSITIVITY);
            var newYRot = Math.Clamp(_cameraRotation.Y + mouseDelta.Y * MOUSE_SENSITIVITY, -MathF.PI * 0.5f, MathF.PI * 0.5f);
            _cameraRotation = new Vector2(newXRot, newYRot);

            var rotation = Quaternion.CreateFromYawPitchRoll(_cameraRotation.X, _cameraRotation.Y, 0f);
            _camPosition += Vector3.Transform(camMovement, rotation);
            _viewMatrix = Matrix.Invert(Matrix.CreateFromQuaternion(rotation) * Matrix.CreateTranslation(_camPosition));
        }


        if (KeyPressed(keyboardState, Keys.I))
        {
            _displayRealTimePlayer = !_displayRealTimePlayer;
        }

        if (KeyPressed(keyboardState, Keys.O))
        {
            _displayClientPlayers = !_displayClientPlayers;
        }

        if (KeyPressed(keyboardState, Keys.P))
        {
            _displaySmoothedPlayer = !_displaySmoothedPlayer;
        }


        if (KeyPressed(keyboardState, Keys.R))
        {
            _gameStateManager = new GameStateManager(1000f / TICKS_PER_SECOND, MAX_ROLLBACK_TIME, PLAYER_COUNT,
                WIGGLE_FREQUENCY);
            _resycSmoothing = new ResycSmoothing(_gameStateManager.Latest.Players, SMOOTHING_LERP, SMOOTHING_LINEAR);
        }
            
        if (KeyPressed(keyboardState, Keys.C)) 
            _controlAll = !_controlAll;
            
        if (KeyPressed(keyboardState, Keys.Up))
            _delay = Math.Min(_delay + 1, _gameStateManager.MaxRollbackTicks);
            
        if (KeyPressed(keyboardState, Keys.Down))
            _delay = Math.Max(_delay - 1, 0);

        var deltaTime = (float)gameTime.ElapsedGameTime.TotalMilliseconds;
            

        var playerInput = keyboardState.IsKeyDown(Keys.Space) ? new PlayerInput() : PlayerInput.CreatePlayerInput(keyboardState, mouseDelta);
            
        (_gameState, _realTimeGameState) =
            _gameStateManager.UpdateCurrentGameState(deltaTime, playerInput, _controlAll);
            
        if (_delay > 0)
            _delayedGameState = _gameStateManager.GetDelayedGameState(_delay);
        else
            _delayedGameState = _gameState;
            
        _smoothedPlayers = _resycSmoothing.SmoothPlayers(_delayedGameState.Players, deltaTime);


        _previousMouseState = mouseState;
        _previousKeyboardState = keyboardState;

        base.Update(gameTime);
    }

    public bool KeyPressed(KeyboardState keyboardState, Keys k)
    {
        return keyboardState.IsKeyDown(k) && !_previousKeyboardState.IsKeyDown(k);
    }

    protected override void Draw(GameTime gameTime)
    {
        GraphicsDevice.Clear(Color.CornflowerBlue);

        var mesh = _bunnyModel.Meshes.First();
        var effect = (BasicEffect)mesh.Effects.First();

        effect.EnableDefaultLighting();
        effect.DiffuseColor = new Vector3(1f, 0.5f, 0f);
        //effect.AmbientLightColor = new Vector3(1f, 0, 0);
        effect.World = Matrix.CreateWorld(_bunnyPosition, Vector3.Forward, Vector3.Up);
        effect.Projection = _projectionMatrix;
        effect.View = _viewMatrix;

        void DrawPlayer(PlayerState playerState, Vector3 colour)
        {
            // effect.World = Matrix.CreateWorld(player.Position, Vector3.Forward, Vector3.Up);
            var rotation = Quaternion.CreateFromYawPitchRoll(playerState.Rotation.X, playerState.Rotation.Y, 0f);
             effect.World = Matrix.CreateScale(0.1f) * Matrix.CreateFromQuaternion(rotation) * Matrix.CreateTranslation(playerState.Position);

            //effect.World = Matrix.CreateWorld(playerState.Position, Vector3.Forward, Vector3.Up);
            effect.DiffuseColor = colour;
            mesh.Draw();
        }

        void DrawPlayers(PlayerState[] players, float brightness)
        {
            var rand = new Random(1);

            for (var index = 1; index < players.Length; index++)
            {
                var player = players[index];
                var colour = new Vector3((float)rand.NextDouble() * brightness,
                    (float)rand.NextDouble() * brightness,
                    (float)rand.NextDouble() * brightness);
                DrawPlayer(player, colour);
            }
        }

        DrawPlayer(_gameState.Players[0], Vector3.One);

        if (_displayRealTimePlayer) 
            DrawPlayers(_realTimeGameState.Players, 0.25f);

        if (_displayClientPlayers)
            DrawPlayers(_gameState.Players, 0.5f);

        if (_displaySmoothedPlayer)
            DrawPlayers(_smoothedPlayers, 1f);

        base.Draw(gameTime);
    }
}