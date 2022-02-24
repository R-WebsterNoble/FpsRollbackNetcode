using Blotch;
using GameLogic;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Content;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;

namespace FpsRollbackNetcode;

/// <summary>
///     A 3D Window
/// </summary>
public class Game : BlWindow3D
{
    private const float TICKS_PER_SECOND = 128f;

    //private const float MAX_ROLLBACK_FRAMES = 10f;// 10 frames
    //private const float MAX_ROLLBACK_TIME = (1000f / TICKS_PER_SECOND) * MAX_ROLLBACK_FRAMES;// 10 frames
    private const float MAX_ROLLBACK_TIME = 500f;
    private const int PLAYER_COUNT = 10;

    private const float ACCELERATION = 0.01f / 1000f;
    private const float MAX_VELOCITY = 0.002f;

    private const float SMOOTHING_LERP = 0.01f;

    private const float SMOOTHING_LINEAR = MAX_VELOCITY * 2f;

    public const float WIGGLE_FREQUENCY = 2000f;

    private readonly float _skyboxDiameter = 1000000f;
    // private TimeSpan _frameProcTime;

    public readonly Random Rand = new(0);
    private bool _controlAll;

    private GameState _gameState;

    private GameStateManager _gameStateManager;
    private BlSprite _hudBackground;

    private KeyboardState _keyboardState;
    // private BlSprite _model = null;

    private Matrix _lastProjectionMatrix;
    private SpriteFont _font;

    private MouseState _mouseState;
    private KeyboardState _previousKeyboardState;
    private GameState _realTimeGameState;
    private GameState _delayedGameState;

    private ResycSmoothing _resycSmoothing;
    private BlSprite _skybox;
    private Model _skyboxModel;
    private PlayerState[] _smoothedPlayers;
    private BlSprite _topHudSprite;


//         private string _help = @"
// Camera controls:
// Dolly  -  Wheel
// Zoom   -  Left-CTRL-wheel
// Truck  -  Left-drag 
// Rotate -  Right-drag
// Pan    -  Left-ALT-left-drag
// Reset  -  Esc
// Fine control  -  Left-Shift
// ";

    private BlSprite _topSprite;
    private int _delay;

    public Game()
    {
        PlayerSimulation.MaxVelocity = MAX_VELOCITY;
        PlayerSimulation.Acceleration = ACCELERATION;
    }

    /// <summary>
    ///     'Setup' is automatically called one time just after the object is created, by the 3D thread.
    ///     You can load fonts, load models, and do other time consuming one-time things here that must be done
    ///     by the object's thread..
    ///     You can also load content later if necessary (like in the Update or Draw methods), but try
    ///     to load them as few times as necessary because loading things takes time.
    /// </summary>
    protected override void Setup()
    {
        _topSprite = new BlSprite(Graphics, "topSprite");
        _topHudSprite = new BlSprite(Graphics, "topHudSprite");
        _hudBackground = new BlSprite(Graphics, "HudBackground");
        Graphics.ClearColor = new Color();
        //Graphics.AutoRotate = .002;

        // Any type of content (3D models, fonts, images, etc.) can be converted to an XNB file by downloading and
        // using the mgcb-editor (see Blotch3D.chm for details). XNB files are then normally added to the project
        // and loaded as shown here. 'Content', here, is the folder that contains the XNB files or subfolders with
        // XNB files. We need to create one ContentManager object for each top-level content folder we'll be loading
        // XNB files from. You can create multiple content managers if content is spread over diverse folders. Some
        // content can also be loaded in its native format using platform specific code (may not be portable) or
        // certain Blotch3D/Monogame methods, like BlGraphicsDeviceManager.LoadFromImageFile.
        Content = new ContentManager(Services, "Content");

        _font = Content.Load<SpriteFont>("Arial14");


        //var cube = new BlSprite(Graphics, "cube");
        //cube.LODs.Add(Content.Load<Model>("cube"));
        //TopSprite.Add("cube", cube);

        //var bunny = new BlSprite(Graphics, "bunny");
        //bunny.LODs.Add(Content.Load<Model>("bunny"));
        //TopSprite.Add("bunny", bunny);
        //bunny.SetAllMaterialBlack();
        //bunny.Color= new Vector3(1, 0, 0);


        //var bunny = Content.Load<Model>("bunny");
        //TopSprite.Add("bunny", bunny);

        //var floor = new BlSprite(Graphics, "floor");
        // var plane = Content.Load<Model>("Plane");
        // var sphere = Content.Load<Model>("uv_sphere_192x96");
        // var myTexture = Graphics.LoadFromImageFile("Content/image_with_alpha.png");

        //
        // Create floor
        //
        //floor.LODs.Add(plane);
        //floor.Mipmap = new BlMipmap(Graphics, MyTexture);
        //floor.SetAllMaterialBlack();
        //floor.EmissiveColor = new Vector3(1, 1, 1);
        //TopSprite.Add("floor", floor);

        //
        // Create parent
        //
        var modelParent = new BlSprite(Graphics, "parent");
        modelParent.Matrix *= Matrix.CreateTranslation(1, 1, 1);
        _topSprite.Add("modelParent", modelParent);

        ////
        //// Create model
        ////
        //Model = new BlSprite(Graphics, "model");
        //Model.Mipmap = new BlMipmap(Graphics, MyTexture);
        //Model.Matrix = Microsoft.Xna.Framework.Matrix.CreateScale(.12f);
        //Model.SetAllMaterialBlack();
        //Model.EmissiveColor = new Vector3(1, 1, 1);
        //modelParent.Add("model", Model);

        //var verts = new VertexPositionNormalTexture[6];
        //var norm = new Vector3(0, 0, 1);
        //verts[0].Position = new Vector3(-1, -1, 0);
        //verts[0].TextureCoordinate = new Vector2(0, 0);
        //verts[0].Normal = norm;

        //verts[1].Position = new Vector3(-1, 1, 0);
        //verts[1].TextureCoordinate = new Vector2(0, 1);
        //verts[1].Normal = norm;

        //verts[2].Position = new Vector3(1, -1, 0);
        //verts[2].TextureCoordinate = new Vector2(1, 0);
        //verts[2].Normal = norm;

        //verts[3].Position = verts[1].Position;
        //verts[3].TextureCoordinate = new Vector2(0, 1);
        //verts[3].Normal = norm;

        //verts[4].Position = new Vector3(1, 1, 0);
        //verts[4].TextureCoordinate = new Vector2(1, 1);
        //verts[4].Normal = norm;

        //verts[5].Position = verts[2].Position;
        //verts[5].TextureCoordinate = new Vector2(1, 0);
        //verts[5].Normal = norm;

        //var vertBuf = BlGeometry.TrianglesToVertexBuffer(Graphics.GraphicsDevice, verts);

        //Model.LODs.Add(sphere);
        //Model.LODs.Add(vertBuf);
        //Model.LODs.Add(sphere);
        //Model.LODs.Add(vertBuf);
        //Model.LODs.Add(null);
        //Model.BoundSphere = new BoundingSphere(Vector3.Zero, 1);


        ////
        //// Create text
        ////
        //var text = new BlSprite(Graphics, "text");
        //text.SphericalBillboard = true;
        //text.ConstSize = true;
        //modelParent.Add("text", text);

        //// Note that in-world textures with alpha (like this one) really need to use
        //// an alpha test to work correctly (see the SpriteAlphaTexture demo)
        //// This one works because it is drawn last and there is no other alpha texture in front of it.
        //var title = new BlSprite(Graphics, "title");
        //title.LODs.Add(Content.Load<Model>("Plane"));
        //title.Matrix = Matrix.CreateScale(.15f, .05f, .15f);
        //title.Mipmap = new BlMipmap(Graphics, Graphics.TextToTexture("These words are\nin world space.\nThis object has LODs", Font, Microsoft.Xna.Framework.Color.Red, Microsoft.Xna.Framework.Color.Transparent));
        //title.MipmapScale = -1000;
        //title.SetAllMaterialBlack();
        //title.EmissiveColor = new Vector3(1, 1, 1);
        //title.PreDraw = (s) =>
        //{
        //	// Disable depth testing
        //	Graphics.GraphicsDevice.DepthStencilState = Graphics.DepthStencilStateDisabled;
        //	return BlSprite.PreDrawCmd.Continue;
        //};
        //title.DrawCleanup = (s) =>
        //{
        //	// Disable depth testing
        //	Graphics.GraphicsDevice.DepthStencilState = Graphics.DepthStencilStateEnabled;
        //};
        //text.Add("title", title);

        ////
        //// Create hud
        ////
        //HudBackground.IncludeInAutoClipping = false;
        //HudBackground.ConstSize = true;
        //TopHudSprite.Add("hudBackground", HudBackground);

        //var myHud = new BlSprite(Graphics, "myHud");
        //HudBackground.Add("myHud", myHud);

        //myHud.Matrix = Matrix.CreateScale(.2f, .1f, .2f);
        //myHud.Matrix *= Matrix.CreateTranslation(3, 1, 0);

        //myHud.LODs.Add(Content.Load<Model>("Plane"));
        //myHud.Mipmap = new BlMipmap(Graphics, Graphics.TextToTexture("HUD text", Font, Color.White, Color.Transparent), 1);
        //myHud.SetAllMaterialBlack();
        //myHud.EmissiveColor = new Vector3(1, 1, 1);

        // Create skybox, with a FrameProc that keeps it centered on the camera
        _skybox = new BlSprite(Graphics, "Skybox", s => { s.Matrix.Translation = Graphics.TargetEye; });
        _skybox.Mipmap = new BlMipmap(Graphics, Graphics.LoadFromImageFile("Content/Skybox.jpg"), 1);
        _skyboxModel = Content.Load<Model>("uv_sphere_192x96");
        _skybox.LODs.Add(_skyboxModel);

        // Exclude from auto-clipping
        _skybox.IncludeInAutoClipping = false;

        // The sphere model is rotated a bit to avoid distortion at the poles. So we have to rotate it back
        _skybox.Matrix = Matrix.CreateFromYawPitchRoll(.462f, 0, .4504f);

        _skybox.Matrix *= Matrix.CreateScale(_skyboxDiameter);
        _skybox.PreDraw = _ =>
        {
            // Set inside facets to visible, rather than outside
            Graphics.GraphicsDevice.RasterizerState = RasterizerState.CullClockwise;

            // Disable depth testing
            Graphics.GraphicsDevice.DepthStencilState = Graphics.DepthStencilStateDisabled;

            // Create a separate View matrix when drawing the skybox, which is the same as the current view matrix but with very high farclip
            _lastProjectionMatrix = Graphics.Projection;
            Graphics.Projection = Matrix.CreatePerspectiveFieldOfView(MathHelper.ToRadians((float)Graphics.Zoom),
                (float)Graphics.CurrentAspect, _skyboxDiameter / 100, _skyboxDiameter * 100);

            return BlSprite.PreDrawCmd.Continue;
        };
        _skybox.DrawCleanup = _ =>
        {
            // restore default settings

            Graphics.GraphicsDevice.DepthStencilState = Graphics.DepthStencilStateEnabled;
            Graphics.GraphicsDevice.RasterizerState = RasterizerState.CullCounterClockwise;
            Graphics.Projection = _lastProjectionMatrix;
        };
        _skybox.SpecularColor = Vector3.Zero;
        _skybox.Color = Vector3.Zero;
        _skybox.EmissiveColor = new Vector3(1, 1, 1);

        //var guiCtrl = new BlGuiControl(this)
        //{
        //	Texture = Graphics.TextToTexture("Click me for a console message", Font, Color.Green, Color.Transparent),
        //	Position = new Vector2(600, 100),
        //	OnMouseOver = (ctrl) =>
        //	{
        //		if (ctrl.PrevMouseState.LeftButton == ButtonState.Released && Mouse.GetState().LeftButton == ButtonState.Pressed)
        //			Console.WriteLine("GUI button was clicked");
        //	}
        //};


        //GuiControls.TryAdd("MyControl", guiCtrl);

        Graphics.FramePeriod = 0;

        //Graphics.FramePeriod = 0.25f;

        //IsFixedTimeStep = true;
        //float maxRollbackTime = (1000f / 100f) * 10f;// 10 frames
        _gameStateManager =
            new GameStateManager(1000f / TICKS_PER_SECOND, MAX_ROLLBACK_TIME, PLAYER_COUNT, WIGGLE_FREQUENCY);

        _resycSmoothing = new ResycSmoothing(_gameStateManager.Latest.Players, SMOOTHING_LERP, SMOOTHING_LINEAR);

        //GameState = new GameState() 
        //{
        //	Players = Enumerable.Range(0, 10).Select(i => new PlayerState() { Position = new Vector3((float)Rand.NextDouble()-0.5f, (float)Rand.NextDouble()-0.5f, 0f) }).ToArray()
        //};

        var bunnyModel = Content.Load<Model>("bunny");

        var playerBunny = new BlSprite(Graphics, "player0");
        playerBunny.LODs.Add(bunnyModel);
        playerBunny.SetAllMaterialBlack();
        playerBunny.Color = new Vector3(float.MaxValue, float.MaxValue, float.MaxValue);

        _topSprite.Add(playerBunny);

        for (var i = 1; i < _gameStateManager.PlayerCount; i++)
        {
            var colour = new Vector3((float)Rand.NextDouble(), (float)Rand.NextDouble(), (float)Rand.NextDouble());

            //var bunny = new BlSprite(Graphics, "player" + i);
            //bunny.LODs.Add(bunnyModel);
            //bunny.SetAllMaterialBlack();
            //bunny.Color = colour;
            //TopSprite.Add(bunny);

            var realTimeBunny = new BlSprite(Graphics, "realTimePlayer" + i);
            realTimeBunny.LODs.Add(bunnyModel);
            realTimeBunny.SetAllMaterialBlack();
            realTimeBunny.Color = colour;
            _topSprite.Add(realTimeBunny);

            var smoothedBunny = new BlSprite(Graphics, "smoothedPlayer" + i);
            smoothedBunny.LODs.Add(bunnyModel);
            smoothedBunny.SetAllMaterialBlack();
            smoothedBunny.Color = colour;
            _topSprite.Add(smoothedBunny);
        }

        for (var i = 1; i < _gameStateManager.PlayerCount; i++)
        {
            var playerRollbackTicks = _gameStateManager.GetDelayedPlayerRollbackTicks(i);
            Console.WriteLine($"player : {i} = {playerRollbackTicks * _gameStateManager.TickDuration}");
        }
    }

    protected override void FrameProc(GameTime timeInfo)
    {
        _keyboardState = Keyboard.GetState();
        _mouseState = Mouse.GetState();

        Graphics.DoDefaultGui();

        if (KeyPressed(Keys.R))
        {
            _gameStateManager = new GameStateManager(1000f / TICKS_PER_SECOND, MAX_ROLLBACK_TIME, PLAYER_COUNT,
                WIGGLE_FREQUENCY);
            _resycSmoothing = new ResycSmoothing(_gameStateManager.Latest.Players, SMOOTHING_LERP, SMOOTHING_LINEAR);
        }

        if (KeyPressed(Keys.C)) 
            _controlAll = !_controlAll;

        if (KeyPressed(Keys.Up))
            _delay = Math.Min(_delay + 1, _gameStateManager.MaxRollbackTicks);

        if (KeyPressed(Keys.Down))
            _delay = Math.Max(_delay - 1, 0);

        var deltaTime = (float)timeInfo.ElapsedGameTime.TotalMilliseconds;


        var playerInput = PlayerInput.CreatePlayerInput(_keyboardState);
        (_gameState, _realTimeGameState) =
            _gameStateManager.UpdateCurrentGameState(deltaTime, playerInput, _controlAll);

        if (_delay > 0)
            _delayedGameState = _gameStateManager.GetDelayedGameState(_delay);
        else
            _delayedGameState = _gameState;

        _smoothedPlayers = _resycSmoothing.SmoothPlayers(_delayedGameState.Players, deltaTime);

        // _frameProcTime = timeInfo.ElapsedGameTime;

        Graphics.CameraSpeed = 1f;
        if (_keyboardState.IsKeyDown(Keys.Space))
        {
            var windowCenter = new Point(Window.ClientBounds.Width / 2, Window.ClientBounds.Height / 2);
            var mouseDelta = windowCenter - _mouseState.Position;

            Mouse.SetPosition(windowCenter.X, windowCenter.Y);

            if (!KeyPressed(Keys.Space)) // Don't move camera on first frame
                Graphics.AdjustCameraPan(mouseDelta.X * MathF.PI * 2, mouseDelta.Y * MathF.PI * 2);
        }

        if (KeyPressed(Keys.E))
            Graphics.AdjustCameraPan(MathF.PI / 2 * 1000f);

        _previousKeyboardState = _keyboardState;
    }

    public bool KeyPressed(Keys k)
    {
        return _keyboardState.IsKeyDown(k) && !_previousKeyboardState.IsKeyDown(k);
    }

    public bool KeyReleased(Keys k)
    {
        return _keyboardState.IsKeyUp(k) && _previousKeyboardState.IsKeyDown(k);
    }

    /// <summary>
    ///     'FrameDraw' is automatically called once per frame if there is enough CPU. Otherwise its called more slowly.
    ///     This is where you would typically draw the scene.
    /// </summary>
    /// <param name="timeInfo">Provides a snapshot of timing values.</param>
    protected override void FrameDraw(GameTime timeInfo)
    {
        //Thread.Sleep(100);
        //Console.WriteLine(timeInfo.ElapsedGameTime.TotalMilliseconds);
        // handle the standard mouse and key commands for controlling the 3D view
        // var mouseRay = Graphics.DoDefaultGui();

        if (Graphics.Zoom > 150)
            Graphics.Zoom = 150;

        // Did user CTRL-leftClick in the 3D display?
        //if (mouseRay != null)
        //{
        //	// search the sprite tree for sprites that had a radius within the selection ray
        //	var sprites = TopSprite.GetRayIntersections((Ray)mouseRay);
        //	foreach (var s in sprites)
        //		Console.WriteLine(s);
        //}

        _skybox.Draw();

        var hudDist = (float)-(Graphics.CurrentNearClip + Graphics.CurrentFarClip) / 2;
        _hudBackground.Matrix = Matrix.CreateScale(.4f, .4f, .4f) * Matrix.CreateTranslation(0, 0, hudDist);

        _topSprite["player0"].Matrix = Matrix.CreateRotationX(MathF.PI / 2f) *
                                       Matrix.CreateTranslation(_gameState.Players[0].Position);

        for (var i = 1; i < _gameState.Players.Length; i++)
        {
            //TopSprite["player" + i].Matrix = Matrix.CreateRotationX(MathF.PI / 2f) * Matrix.CreateTranslation(GameState.Players[i].Position);

            _topSprite["realTimePlayer" + i].Matrix = Matrix.CreateRotationX(MathF.PI / 2f) *
                                                      Matrix.CreateTranslation(_realTimeGameState.Players[i].Position);

            _topSprite["smoothedPlayer" + i].Matrix = Matrix.CreateRotationX(MathF.PI / 2f) *
                                                      Matrix.CreateTranslation(_smoothedPlayers[i].Position);
        }

        _topSprite.Draw();

        Graphics.SetSpriteToCamera(_topHudSprite);
        Graphics.GraphicsDevice.DepthStencilState = DepthStencilState.None;
        _topHudSprite.Draw();
        Graphics.GraphicsDevice.DepthStencilState = DepthStencilState.Default;


        //var MyMenuText = String.Format("{0}\nEye: {1}\nLookAt: {2}\nMaxDistance: {3}\nMinDistance: {4}\nViewAngle: {5}\nModelLod: {6}\nModelApparentSize: {7}",
        //	Help,
        //	Graphics.Eye,
        //	Graphics.LookAt,
        //	Graphics.MaxCamDistance,
        //	Graphics.MinCamDistance,
        //	Graphics.Zoom,
        //	Model.LodTarget,
        //	Model.ApparentSize
        //);

        //var MyMenuText = $"{ Help}\n" +
        //    $"Eye: { Graphics.Eye}\n" +
        //    $"LookAt: { Graphics.LookAt}\n" +
        //    $"MaxDistance: { (object)Graphics.MaxCamDistance}\n" +
        //    $"MinDistance: { (object)Graphics.MinCamDistance}\n" +
        //    $"ViewAngle: { (object)Graphics.Zoom}\n" +
        //    $"ModelLod: { (object)Model.LodTarget}\n" +
        //    $"ModelApparentSize: { (object)Model.ApparentSize }";

        //var MyMenuText = $"FrameProcTime: {_frameProcTime.TotalMilliseconds:0.0000}, ({1f / _frameProcTime.TotalSeconds:0.00})\n" +
        //    $"FrameDraw: { timeInfo.ElapsedGameTime.TotalMilliseconds:0.0000}, ({1f / timeInfo.ElapsedGameTime.TotalSeconds:0.00})";

        // var myMenuText = $"Position: {_gameState.Players[0].Position.LengthSquared():0.00000}\n" +
        //  $"Velocity: {_gameState.Players[0].Velocity.LengthSquared()}\n" +
        //  $"{_gameState.Players[0].Velocity == Vector3.Zero}";

        var myMenuText = $"Delay: {_delay*_gameStateManager.TickDuration}ms";

        try
        {
            // handle undrawable characters for the specified font(like the infinity symbol)
            try
            {
                Graphics.DrawText(myMenuText, _font, new Vector2(50, 50));
            }
            catch
            {
                // ignored
            }
        }
        catch (Exception e)
        {
            Console.WriteLine(e);
        }

        /*
        Console.WriteLine("{0}  {1}  {2}  {3}  {4}",
            graphics.CurrentNearClip,
            graphics.CurrentFarClip,
            graphics.MinCamDistance,
            graphics.MaxCamDistance,
            hudDist
        );
        */
        //Console.WriteLine(model.LodCurrentIndex);
    }
}