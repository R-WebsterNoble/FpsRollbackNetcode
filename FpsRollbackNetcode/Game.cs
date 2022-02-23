using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Content;
using Blotch;
using Microsoft.Xna.Framework.Input;
using GameLogic;

namespace FpsRollbackNetcode
{
    /// <summary>
    /// A 3D Window
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


		string Help = @"
Camera controls:
Dolly  -  Wheel
Zoom   -  Left-CTRL-wheel
Truck  -  Left-drag 
Rotate -  Right-drag
Pan    -  Left-ALT-left-drag
Reset  -  Esc
Fine control  -  Left-Shift
";

		BlSprite TopSprite = null;
		BlSprite TopHudSprite = null;
		BlSprite HudBackground = null;
		BlSprite Skybox = null;
		BlSprite Model = null;

		Matrix LastProjectionMatrix;
		Model SkyboxModel = null;
		float SkyboxDiameter = 1000000f;
		SpriteFont Font;

		private MouseState _mouseState;
		private MouseState _previousMouseState;
		private KeyboardState _keyboardState;
		private KeyboardState _previousKeyboardState;
        private PlayerState[] SmoothedPlayers;
        private TimeSpan _frameProctime;

		public readonly Random Rand = new Random(0);

        public GameStateManager GameStateManager { get; private set; }

        private ResycSmoothing ResycSmoothing;

        public GameState RealTimeGameState { get; private set; }

        private GameState GameState;
        private int FrameNum;

        public Game()
		{
			PlayerSimulation.MAX_VELOCITY = MAX_VELOCITY;
			PlayerSimulation.ACCELERATION = ACCELERATION;
		}

		/// <summary>
		/// 'Setup' is automatically called one time just after the object is created, by the 3D thread.
		/// You can load fonts, load models, and do other time consuming one-time things here that must be done
		/// by the object's thread..
		/// You can also load content later if necessary (like in the Update or Draw methods), but try
		/// to load them as few times as necessary because loading things takes time.
		/// </summary>
		protected override void Setup()
		{
			TopSprite = new BlSprite(Graphics, "topSprite");
			TopHudSprite = new BlSprite(Graphics, "topHudSprite");
			HudBackground = new BlSprite(Graphics, "HudBackground");
			Graphics.ClearColor = new Microsoft.Xna.Framework.Color();
			//Graphics.AutoRotate = .002;

			// Any type of content (3D models, fonts, images, etc.) can be converted to an XNB file by downloading and
			// using the mgcb-editor (see Blotch3D.chm for details). XNB files are then normally added to the project
			// and loaded as shown here. 'Content', here, is the folder that contains the XNB files or subfolders with
			// XNB files. We need to create one ContentManager object for each top-level content folder we'll be loading
			// XNB files from. You can create multiple content managers if content is spread over diverse folders. Some
			// content can also be loaded in its native format using platform specific code (may not be portable) or
			// certain Blotch3D/Monogame methods, like BlGraphicsDeviceManager.LoadFromImageFile.
			Content = new ContentManager(Services, "Content");

			Font = Content.Load<SpriteFont>("Arial14");


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

			var floor = new BlSprite(Graphics, "floor");
			var plane = Content.Load<Model>("Plane");
			var sphere = Content.Load<Model>("uv_sphere_192x96");
			var MyTexture = Graphics.LoadFromImageFile("Content/image_with_alpha.png");

			//
			// Create floor
			//
			floor.LODs.Add(plane);
			floor.Mipmap = new BlMipmap(Graphics, MyTexture);
			floor.SetAllMaterialBlack();
			floor.EmissiveColor = new Vector3(1, 1, 1);
			TopSprite.Add("floor", floor);

			//
			// Create parent
			//
			var modelParent = new BlSprite(Graphics, "parent");
			modelParent.Matrix *= Matrix.CreateTranslation(1, 1, 1);
			TopSprite.Add("modelParent", modelParent);

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
			Skybox = new BlSprite(Graphics, "Skybox", (s) =>
			{
				s.Matrix.Translation = Graphics.TargetEye;
			});
			Skybox.Mipmap = new BlMipmap(Graphics, Graphics.LoadFromImageFile("Content/Skybox.jpg"), 1);
			SkyboxModel = Content.Load<Model>("uv_sphere_192x96");
			Skybox.LODs.Add(SkyboxModel);

			// Exclude from auto-clipping
			Skybox.IncludeInAutoClipping = false;

			// The sphere model is rotated a bit to avoid distortion at the poles. So we have to rotate it back
			Skybox.Matrix = Matrix.CreateFromYawPitchRoll(.462f, 0, .4504f);

			Skybox.Matrix *= Matrix.CreateScale(SkyboxDiameter);
			Skybox.PreDraw = (s) =>
			{
				// Set inside facets to visible, rather than outside
				Graphics.GraphicsDevice.RasterizerState = RasterizerState.CullClockwise;

				// Disable depth testing
				Graphics.GraphicsDevice.DepthStencilState = Graphics.DepthStencilStateDisabled;

				// Create a separate View matrix when drawing the skybox, which is the same as the current view matrix but with very high farclip
				LastProjectionMatrix = Graphics.Projection;
				Graphics.Projection = Microsoft.Xna.Framework.Matrix.CreatePerspectiveFieldOfView(MathHelper.ToRadians((float)Graphics.Zoom), (float)Graphics.CurrentAspect, SkyboxDiameter / 100, SkyboxDiameter * 100);

				return BlSprite.PreDrawCmd.Continue;
			};
			Skybox.DrawCleanup = (s) =>
			{
				// retore default settings

				Graphics.GraphicsDevice.DepthStencilState = Graphics.DepthStencilStateEnabled;
				Graphics.GraphicsDevice.RasterizerState = RasterizerState.CullCounterClockwise;
				Graphics.Projection = LastProjectionMatrix;
			};
			Skybox.SpecularColor = Vector3.Zero;
			Skybox.Color = Vector3.Zero;
			Skybox.EmissiveColor = new Vector3(1, 1, 1);

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
			GameStateManager = new GameStateManager(1000f / TICKS_PER_SECOND, MAX_ROLLBACK_TIME, PLAYER_COUNT, WIGGLE_FREQUENCY);

			ResycSmoothing = new ResycSmoothing(GameStateManager.Latest.Players, SMOOTHING_LERP, SMOOTHING_LINEAR);

			//GameState = new GameState() 
			//{
			//	Players = Enumerable.Range(0, 10).Select(i => new PlayerState() { Position = new Vector3((float)Rand.NextDouble()-0.5f, (float)Rand.NextDouble()-0.5f, 0f) }).ToArray()
			//         };

			var bunnyModel = Content.Load<Model>("bunny");

			var initialGameState = GameStateManager.Latest;
			for (int i = 0; i < initialGameState.Players.Length; i++)
			{
				//var player = initialGameState.Players[i];

				Vector3 colour = new Vector3((float)Rand.NextDouble(), (float)Rand.NextDouble(), (float)Rand.NextDouble());

				var bunny = new BlSprite(Graphics, "player" + i);
				bunny.LODs.Add(bunnyModel);
				bunny.SetAllMaterialBlack();
                bunny.Color = colour;

				TopSprite.Add(bunny);

				var realTimebunny = new BlSprite(Graphics, "realTimePlayer" + i);
				realTimebunny.LODs.Add(bunnyModel);
				realTimebunny.SetAllMaterialBlack();
				realTimebunny.Color = colour;
				TopSprite.Add(realTimebunny);

				var smoothedBuny = new BlSprite(Graphics, "smoothedPlayer" + i);
				smoothedBuny.LODs.Add(bunnyModel);
				smoothedBuny.SetAllMaterialBlack();
				smoothedBuny.Color = colour;
				TopSprite.Add(smoothedBuny);
			}

			GameStateManager.Start();

			for (int i = 1; i < GameStateManager.PlayerCount; i++)
			{
				var playerRollbackTicks = GameStateManager.GetDelayedPlayerRollbackTicks(i);
				Console.WriteLine($"player : {i} = {playerRollbackTicks * GameStateManager.TickDuration}");
			}
		}

        protected override void FrameProc(GameTime timeInfo)
		{
			_keyboardState = Keyboard.GetState();
			_mouseState = Mouse.GetState();

			if (KeyPressed(Keys.R))
			{
				GameStateManager = new GameStateManager(1000f / TICKS_PER_SECOND, MAX_ROLLBACK_TIME, PLAYER_COUNT, WIGGLE_FREQUENCY);
				ResycSmoothing = new ResycSmoothing(GameStateManager.Latest.Players, SMOOTHING_LERP, SMOOTHING_LINEAR);
			}

			FrameNum++;

            //timeInfo = new GameTime 
            //{
            //	TotalGameTime = TimeSpan.FromMilliseconds(FrameNum * GameStateManager.TickRate),
            //	ElapsedGameTime = TimeSpan.FromMilliseconds(GameStateManager.TickRate)
            //};

            ////if ((FrameNum + 1) % 100 == 1)
            ////{
            ////    //var rollbackTo = GameStateManager.TickNum - 5;// GameStateManager.MaxRollbackTicks;
            ////    var rollbackTo = GameStateManager.TickNum - (Math.Min(GameStateManager.MaxRollbackTicks, GameStateManager.TickNum));

            ////    //GameStateManager.UpdateRollbackGameState(rollbackTo, 1, new PlayerInput { playerActions = PlayerAction.MoveForward });
            ////    GameStateManager.LatestInputs[1] = new PlayerInput { playerActions = GameStateManager.LatestInputs[1].playerActions ^ PlayerAction.MoveForward };
            ////    //inputs.playerActions = inputs.playerActions ^ PlayerAction.MoveForward;
            ////    GameStateManager.UpdateRollbackGameState(rollbackTo, 1, GameStateManager.LatestInputs[1]);
            ////}
            float deltaTime = (float)timeInfo.ElapsedGameTime.TotalMilliseconds;


			PlayerInput playerInput = PlayerInput.CreatePlayerInput(_keyboardState);
            (GameState, RealTimeGameState) = GameStateManager.UpdateCurrentGameState(deltaTime, playerInput);
			SmoothedPlayers = ResycSmoothing.SmoothPlayers(GameState.Players,deltaTime);

			//RealTimeGameState = PlayerSimulator.SimulatePlayers(deltaTime);
//RealTimeGameState = GameState;
//			PlayerSimulator.SimulatePlayers();


			//GameState = Simulation.Next((float)timeInfo.ElapsedGameTime.TotalMilliseconds, GameState, PlayerInput.CreatePlayerInput(FrameNum));

			_frameProctime = timeInfo.ElapsedGameTime;

			//Console.WriteLine(timeInfo.ElapsedGameTime.TotalMilliseconds);
			Graphics.CameraSpeed = 1f;
			if (_keyboardState.IsKeyDown(Keys.Space))
			{				
				var windowCenter = new Point(Window.ClientBounds.Width / 2, Window.ClientBounds.Height / 2);
				var mouseDelta =  windowCenter - _mouseState.Position;
				//Console.WriteLine(mouseDelta);
				Mouse.SetPosition(windowCenter.X, windowCenter.Y);

				if(!KeyPressed(Keys.Space)) // Don't move camera on first frame
					Graphics.AdjustCameraPan(mouseDelta.X * MathF.PI * 2, mouseDelta.Y * MathF.PI * 2);

				//Graphics.TargetEye = new Vector3(0f, -5f, 5f );
				//Graphics.camer
				//Graphics.TargetEye = Graphics.TargetLookAt;
				//Graphics.LookAt = Graphics.TargetLookAt;

				//Graphics.LookAt = Graphics.TargetLookAt;
				 
				//System.Reflection.PropertyInfo propertyInfo = typeof(BlGraphicsDeviceManager).GetProperty("LookAt");
				//if (propertyInfo != null)
				//{
				//	propertyInfo.SetValue(Graphics, Graphics.TargetLookAt);
				//}
			}

            if (KeyPressed(Keys.E))
            {
                //Graphics.AdjustCameraRotation((MathF.PI/2) * 250f);
                Graphics.AdjustCameraPan((MathF.PI / 2) * 1000f);
               
            }

			_previousMouseState = _mouseState;
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
		/// 'FrameDraw' is automatically called once per frame if there is enough CPU. Otherwise its called more slowly.
		/// This is where you would typically draw the scene.
		/// </summary>
		/// <param name="gameTime">Provides a snapshot of timing values.</param>
		protected override void FrameDraw(GameTime timeInfo)
        {
			//Thread.Sleep(100);
			//Console.WriteLine(timeInfo.ElapsedGameTime.TotalMilliseconds);
			// handle the standard mouse and key commands for controlling the 3D view
			var mouseRay = Graphics.DoDefaultGui();

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

			Skybox.Draw();

			var hudDist = (float)-(Graphics.CurrentNearClip + Graphics.CurrentFarClip) / 2;
			HudBackground.Matrix = Matrix.CreateScale(.4f, .4f, .4f) * Matrix.CreateTranslation(0, 0, hudDist);

			TopSprite["player0"].Matrix = Matrix.CreateRotationX(MathF.PI / 2f) * Matrix.CreateTranslation(GameState.Players[0].Position);

			for (int i = 1; i < GameState.Players.Length; i++)
			{
				//TopSprite["player" + i].Matrix = Matrix.CreateRotationX(MathF.PI / 2f) * Matrix.CreateTranslation(GameState.Players[i].Position);

				TopSprite["realTimePlayer" + i].Matrix = Matrix.CreateRotationX(MathF.PI / 2f) * Matrix.CreateTranslation(RealTimeGameState.Players[i].Position);

				TopSprite["smoothedPlayer" + i].Matrix = Matrix.CreateRotationX(MathF.PI / 2f) * Matrix.CreateTranslation(SmoothedPlayers[i].Position);
			}

			TopSprite.Draw();			

			Graphics.SetSpriteToCamera(TopHudSprite);
			Graphics.GraphicsDevice.DepthStencilState = DepthStencilState.None;
			TopHudSprite.Draw();
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

            //var MyMenuText = $"FrameProcTime: {_frameProctime.TotalMilliseconds:0.0000}, ({1f / _frameProctime.TotalSeconds:0.00})\n" +
            //    $"FrameDraw: { timeInfo.ElapsedGameTime.TotalMilliseconds:0.0000}, ({1f / timeInfo.ElapsedGameTime.TotalSeconds:0.00})";

            var MyMenuText = $"Position: {GameState.Players[0].Position.LengthSquared():0.00000}\n" +
                         $"Velocity: {GameState.Players[0].Velocity.LengthSquared()}\n" +
                         $"{GameState.Players[0].Velocity == Vector3.Zero}";

            try
            {
                // handle undrawable characters for the specified font(like the infinity symbol)
                try
                {
                    Graphics.DrawText(MyMenuText, Font, new Vector2(50, 50));
                }
                catch { }
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
}