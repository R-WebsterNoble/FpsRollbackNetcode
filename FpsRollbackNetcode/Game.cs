using System.Diagnostics;
using System.Runtime.InteropServices;
using BepuPhysics;
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
        
    // private const float ACCELERATION = 0.01f / 1000f;
    private const float MAX_VELOCITY = 0.002f;
        
    private const float SMOOTHING_LERP = 0.0005f;
    private const float SMOOTHING_LINEAR = MAX_VELOCITY * 1.45f;

    private const float SMOOTHING_ROT_LERP = 0.01f;
    private const float SMOOTHING_ROT_LIN = 0.003f;

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
    private static readonly bool IsOsWindows = RuntimeInformation.IsOSPlatform(OSPlatform.Windows);
    private static readonly IntPtr Handle = Process.GetCurrentProcess().MainWindowHandle;
    private StaticHandle _box;
    private BodyHandle[] _spheres;
    private Model _sphereModel;
    private Model _cubeModel;

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
        var bunnyMesh = _bunnyModel.Meshes.First();
        var bunnyEffect = (BasicEffect)bunnyMesh.Effects.First();
        bunnyEffect.EnableDefaultLighting();
        bunnyEffect.DiffuseColor = new Vector3(1f, 0.5f, 0f);
        //effect.AmbientLightColor = new Vector3(1f, 0, 0);
        bunnyEffect.World = Matrix.CreateWorld(_bunnyPosition, Vector3.Forward, Vector3.Up);
        bunnyEffect.Projection = _projectionMatrix;
        bunnyEffect.View = _viewMatrix;

        if (Debugger.IsAttached)
        {
            //workaround for inconsisent frame times when debugging
            IsFixedTimeStep = true;
            TargetElapsedTime = TimeSpan.FromSeconds(1.0 / 144.0);
        }
        else
            IsFixedTimeStep = false;

        _gameStateManager = new GameStateManager(1000f / TICKS_PER_SECOND,
            MAX_ROLLBACK_TIME,
            PLAYER_COUNT,
            WIGGLE_FREQUENCY);

        _resycSmoothing = new ResycSmoothing(_gameStateManager.Latest.Players,
            SMOOTHING_LERP,
            SMOOTHING_LINEAR,
            SMOOTHING_ROT_LERP,
            SMOOTHING_ROT_LIN);
            
        //GameState = new GameState() 
        //{
        //	Players = Enumerable.Range(0, 10).Select(i => new PlayerState() { Position = new Vector3((float)Rand.NextDouble()-0.5f, (float)Rand.NextDouble()-0.5f, 0f) }).ToArray()
        //};
            

        for (var i = 1; i < _gameStateManager.PlayerCount; i++)
        {
            var playerRollbackTicks = _gameStateManager.GetDelayedPlayerRollbackTicks(i);
            Console.WriteLine($"player : {i} = {playerRollbackTicks * _gameStateManager.TickDuration}");
        }

        Physics.Initialise();


        _cubeModel = Content.Load<Model>("cube");
        var cubeMesh = _cubeModel.Meshes.First();
        var cubeEffect = (BasicEffect)cubeMesh.Effects.First();
        cubeEffect.EnableDefaultLighting();
        cubeEffect.DiffuseColor = new Vector3(1f, 0.5f, 0f);
        //effect.AmbientLightColor = new Vector3(1f, 0, 0);
        cubeEffect.World = Matrix.CreateWorld(Vector3.Zero, Vector3.Forward, Vector3.Up);
        cubeEffect.Projection = _projectionMatrix;
        cubeEffect.View = _viewMatrix;

        _box = Physics.AddStaticBox(new System.Numerics.Vector3(0f,-0.5f,0f), System.Numerics.Quaternion.CreateFromYawPitchRoll(0f,0f,0.01f),  20f, 1f, 20f);

        _sphereModel = Content.Load<Model>("icosphere");
        var sphereMesh = _sphereModel.Meshes.First();
        var sphereEffect = (BasicEffect)sphereMesh.Effects.First();
        sphereEffect.EnableDefaultLighting();
        sphereEffect.DiffuseColor = new Vector3(1f, 0.5f, 0f);
        //effect.AmbientLightColor = new Vector3(1f, 0, 0);
        sphereEffect.World = Matrix.CreateWorld(new Vector3(0f, 5f, 0f), Vector3.Forward, Vector3.Up);
        sphereEffect.Projection = _projectionMatrix;
        sphereEffect.View = _viewMatrix;

        _spheres = Enumerable.Range(0, 10)
            .Select(i => Physics.AddBall(new System.Numerics.Vector3(i%2 * 0.1f, -0.5f + i * 1.1f, i % 3 * 0.1f), 0.5f))
            .ToArray();
    }

    protected override void LoadContent()
    {
        _spriteBatch = new SpriteBatch(GraphicsDevice);
    }

    protected override void UnloadContent()
    {

    }

    [DllImport("user32.dll")]
    private static extern bool GetCursorPos(out Point lpPoint);

    [DllImport("user32.dll")]
    private static extern bool SetCursorPos(int X, int Y);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT
    {
        public int Left;        // x position of upper-left corner
        public int Top;         // y position of upper-left corner
        public int Right;       // x position of lower-right corner
        public int Bottom;      // y position of lower-right corner
    }

    protected override void Update(GameTime gameTime)
    {
        var deltaTime = (float)gameTime.ElapsedGameTime.TotalMilliseconds;
        if(deltaTime == 0f)
            return;

        var keyboardState = Keyboard.GetState();
        var mouseState = Mouse.GetState();

        if (GamePad.GetState(PlayerIndex.One).Buttons.Back ==
            ButtonState.Pressed || keyboardState.IsKeyDown(Keys.Escape))
            Exit();
        
        var mouseDelta = Point.Zero;

        if (keyboardState.IsKeyDown(Keys.Space) || mouseState.RightButton == ButtonState.Pressed)
        {
            Point windowCenter;
            if (IsOsWindows)
            {
                GetWindowRect(Handle, out var rct);
                var centerX = rct.Left + (rct.Right - rct.Left) / 2;
                var centerY = rct.Top + (rct.Bottom - rct.Top) / 2;
                windowCenter = new Point(centerX, centerY);
            }
            else
            {
                windowCenter = new Point(Window.ClientBounds.Width / 2, Window.ClientBounds.Height / 2);
            }

            if (_previousKeyboardState.IsKeyDown(Keys.Space) || _previousMouseState.RightButton == ButtonState.Pressed)
            {
                Point cursorPos;
                if(IsOsWindows)
                    GetCursorPos(out cursorPos);
                else
                    cursorPos = mouseState.Position;

                mouseDelta = windowCenter - cursorPos;
            }
            if (IsOsWindows)
                SetCursorPos(windowCenter.X, windowCenter.Y);
            else
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
            _resycSmoothing = new ResycSmoothing(_gameStateManager.Latest.Players, SMOOTHING_LERP, SMOOTHING_LINEAR, 0.01f, 0.003f);
        }
            
        if (KeyPressed(keyboardState, Keys.C)) 
            _controlAll = !_controlAll;
            
        if (KeyPressed(keyboardState, Keys.Up))
            _delay = Math.Min(_delay + 1, _gameStateManager.MaxRollbackTicks);
            
        if (KeyPressed(keyboardState, Keys.Down))
            _delay = Math.Max(_delay - 1, 0);

            

        var playerInput = keyboardState.IsKeyDown(Keys.Space) ? new PlayerInput() : PlayerInput.CreatePlayerInput(keyboardState, mouseDelta);
            
        (_gameState, _realTimeGameState) =
            _gameStateManager.UpdateCurrentGameState(deltaTime, playerInput, _controlAll);
            
        if (_delay > 0)
            _delayedGameState = _gameStateManager.GetDelayedGameState(_delay);
        else
            _delayedGameState = _gameState;
            
        _smoothedPlayers = _resycSmoothing.SmoothPlayers(_delayedGameState.Players, deltaTime);

        Physics.Simulate(deltaTime);

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

        var bunnyMesh = _bunnyModel.Meshes.First();
        var bunnyEffect = (BasicEffect)bunnyMesh.Effects.First();

        // bunnyEffect.EnableDefaultLighting();
        // bunnyEffect.DiffuseColor = new Vector3(1f, 0.5f, 0f);
        // //effect.AmbientLightColor = new Vector3(1f, 0, 0);
        bunnyEffect.World = Matrix.CreateWorld(_bunnyPosition, Vector3.Forward, Vector3.Up);
        bunnyEffect.Projection = _projectionMatrix;
        bunnyEffect.View = _viewMatrix;

        void DrawPlayer(PlayerState playerState, Vector3 colour)
        {
            // effect.World = Matrix.CreateWorld(player.Position, Vector3.Forward, Vector3.Up);
            var rotation = Quaternion.CreateFromYawPitchRoll(playerState.Rotation.X, playerState.Rotation.Y, 0f);
            bunnyEffect.World = Matrix.CreateScale(0.1f) * Matrix.CreateFromQuaternion(rotation) * Matrix.CreateTranslation(playerState.Position);

            //effect.World = Matrix.CreateWorld(playerState.Position, Vector3.Forward, Vector3.Up);
            bunnyEffect.DiffuseColor = colour;
            bunnyMesh.Draw();
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

        void DrawPhysicsObject(Model model, RigidPose pose, Vector3 scale)
        {
            var mesh = model.Meshes.First();
            var effect = (BasicEffect)mesh.Effects.First();
            effect.View = _viewMatrix;
            effect.Projection = _projectionMatrix;

            var position = pose.Position;
            var orientation = pose.Orientation;

            effect.World = Matrix.CreateScale(scale*0.5f) *
                           Matrix.CreateFromQuaternion(To(orientation)) *
                           Matrix.CreateTranslation(To(position));

            mesh.Draw();
        }

        var simulation = Physics.Simulation;

        foreach (var sphere in _spheres)
        {
            var boxPose = simulation.Bodies[sphere].Pose;
            DrawPhysicsObject(_sphereModel, boxPose, Vector3.One);
        }

        var spherePose = simulation.Statics[_box].Pose;
        DrawPhysicsObject(_cubeModel, spherePose, new Vector3(20f, 1f, 20f));


        base.Draw(gameTime);
    }



    static System.Numerics.Vector3 To(Vector3 v)
    {
        return new System.Numerics.Vector3(v.X, v.Y, v.Z);
    }

    static System.Numerics.Quaternion To(Quaternion q)
    {
        return new System.Numerics.Quaternion(q.X, q.Y, q.Z, q.W);
    }

    static Vector3 To(System.Numerics.Vector3 v)
    {
        return new Vector3(v.X, v.Y, v.Z);
    }

    static Quaternion To(System.Numerics.Quaternion q)
    {
        return new Quaternion(q.X, q.Y, q.Z, q.W);
    }
}