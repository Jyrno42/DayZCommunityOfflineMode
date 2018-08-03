#include "$CurrentDir:\\missions\\DayZCommunityOfflineMode.ChernarusPlus\\gui\\WelcomeMenu.c"
#include "$CurrentDir:\\missions\\DayZCommunityOfflineMode.ChernarusPlus\\gui\\PositionMenu.c"
#include "$CurrentDir:\\missions\\DayZCommunityOfflineMode.ChernarusPlus\\gui\\WeatherMenu.c"
#include "$CurrentDir:\\missions\\DayZCommunityOfflineMode.ChernarusPlus\\gui\\ObjectMenu.c"
#include "$CurrentDir:\\missions\\DayZCommunityOfflineMode.ChernarusPlus\\modules\\ObjectEditor.c"
#include "$CurrentDir:\\missions\\DayZCommunityOfflineMode.ChernarusPlus\\modules\\CameraTool.c"
#include "$CurrentDir:\\missions\\DayZCommunityOfflineMode.ChernarusPlus\\modules\\Module.c"
#include "$CurrentDir:\\missions\\DayZCommunityOfflineMode.ChernarusPlus\\modules\\KeyMouseBinding.c"
#include "$CurrentDir:\\missions\\DayZCommunityOfflineMode.ChernarusPlus\\patches\\PluginLifespan.c"
#include "$CurrentDir:\\missions\\DayZCommunityOfflineMode.ChernarusPlus\\patches\\DebugMonitor.c"

#include "$CurrentDir:\\missions\\DayZCommunityOfflineMode.ChernarusPlus\\modules\\SaveModule\\SaveModule.c"

#include "$CurrentDir:\\missions\\DayZCommunityOfflineMode.ChernarusPlus\\modules\\COMKeyBinds.c"

class CommunityOfflineMode : MissionGameplay
{
	protected bool DISABLE_RESPAWN_ONRESTART = true; // disable(true) / enable(false) - Player Respawn on every Mission Restart
	protected bool DISABLE_HIVE = false;	 		// disable(true) / enable(false) - Local Hive / Economy 

	private ref set<ref Module> m_Modules;
	
	PlayerBase m_oPlayer;

	ref PluginLifespanPatched cpl;
	
	//Patches
	protected ref DebugMonitorPatched m_debugMonitorPatched;
	protected ref ObjectEditor m_ObjectEditor;

	//Until we find a better way
	protected bool m_bLoaded = false;

	//For freecam and utils
	protected Camera m_oCamera;
	
	protected bool m_bDebugMonitor = false;
	protected bool m_bGodMode = false;
	protected bool m_bWelcome = false;
	protected int m_nAutoWalkMode = 0;

	// For Aiming like 0.62
	protected bool m_bOldAiming = false;

	//For keyhandler
	protected bool m_IsCtrlHolding = false;
	protected bool m_IsWinHolding = false;
	protected bool m_IsLeftAltHolding = false;
	protected bool m_IsRightAltHolding = false;
	protected bool m_IsLeftShiftHolding = false;
	protected bool m_IsRightShiftHolding = false;
	
	//For mousehandler - keeps track of last press/release time
	ref array<ref MouseButtonInfo> m_MouseButtons; 
	protected const int CLICK_TIME			= 200; //ms
	protected const int HOLD_CLICK_TIME_MIN	= 200; //ms
	protected const int DOUBLE_CLICK_TIME	= 300; //ms

	void CommunityOfflineMode()
	{
		Print( "CommunityOfflineMode::CommunityOfflineMode()" );
		
		m_Modules 	   = new set<ref Module>;
		m_MouseButtons = new array<ref MouseButtonInfo>;
		
		m_MouseButtons.Insert ( new MouseButtonInfo( MouseState.LEFT ) );
		m_MouseButtons.Insert ( new MouseButtonInfo( MouseState.RIGHT ) );
		m_MouseButtons.Insert ( new MouseButtonInfo( MouseState.MIDDLE ) );
		
		// register modules
		RegisterModules();
		
		// for beard
		g_Game.SetMissionState(DayZGame.MISSION_STATE_GAME);
		SetDispatcher(new DispatcherCaller);
		cpl = new PluginLifespanPatched();
	}


	void ~CommunityOfflineMode()
	{
		Print( "CommunityOfflineMode::~CommunityOfflineMode()" );
		
		if(GetHive())
		{
			DestroyHive();
		}
	}


	void RegisterModules()
	{
		m_Modules.Insert( new ObjectEditor(this) );
		m_Modules.Insert( new CameraTool(this) );
		m_Modules.Insert( new MiscFunctions(this) );
		m_Modules.Insert( new COMKeyBinds(this) );
		m_Modules.Insert( new SaveModule(this) );
	}
	
	
	void InitializeModules()
	{
		
		for ( int i = 0; i < m_Modules.Count(); ++i)
		{
			m_Modules.Get(i).Init();
		}
		
	}
	
	
	override void OnInit()
	{
		super.OnInit();

        SetupWeather();

		SpawnPlayer();
		
		InitHive();
		
		InitializeModules();
		
	}
	

	Module GetModule(typename module_Type)
	{
		for ( int i = 0; i < m_Modules.Count(); ++i)
		{
			Module module = m_Modules.Get(i);
			if (module.GetModuleType() == module_Type) 
			{
				return module;
			}
		}
		return NULL;
	}

	
	
	protected MouseButtonInfo GetMouseButtonInfo( int button )
	{	
		for ( int i = 0; i < m_MouseButtons.Count(); ++i )
		{
			MouseButtonInfo info = m_MouseButtons.Get(i);
			
			if ( info.GetButtonID() == button )
			{
				return info;
			}
		}
		return NULL;
	}
	
	
	
	override void OnMissionStart()
	{
		super.OnMissionStart();
	
		for ( int i = 0; i < m_Modules.Count(); ++i)
		{
			m_Modules.Get(i).onMissionStart();
		}
		
		CreateDebugMonitor();
		
		m_debugMonitorPatched.Hide();
	}

	
	
	override void OnMissionFinish()
	{	
		for ( int i = 0; i < m_Modules.Count(); ++i)
		{
			m_Modules.Get(i).onMissionFinish();
		}
		
		super.OnMissionFinish();
	}

	
	
    void OnMissionLoaded()
    {
        GetGame().GetUIManager().ScreenFadeOut( 0 );
		
		for ( int i = 0; i < m_Modules.Count(); ++i)
		{
			m_Modules.Get(i).onMissionLoaded();
		}
    }


	
	override void OnUpdate( float timeslice )
	{
	    super.OnUpdate( timeslice );

        if( !m_bLoaded && !GetDayZGame().IsLoading() )
        {
            m_bLoaded = true;
            OnMissionLoaded();
        }

		if( m_bGodMode )
		{
			m_oPlayer.SetHealth( m_oPlayer.GetMaxHealth( "", "" ) );
			m_oPlayer.SetHealth( "","Blood", m_oPlayer.GetMaxHealth( "", "Blood" ) );
			// m_oPlayer.SetStamina(1000,1000);
			m_oPlayer.GetStatStamina().Set(1000);
			m_oPlayer.GetStatEnergy().Set(1000);
			m_oPlayer.GetStatWater().Set(1000);
			m_oPlayer.GetStatStomachSolid().Set(300);		
			m_oPlayer.GetStatStomachWater().Set(300);
			m_oPlayer.GetStatStomachEnergy().Set(300);
			m_oPlayer.GetStatHeatComfort().Set(0);
			
			EntityAI oWeapon = m_oPlayer.GetHumanInventory().GetEntityInHands();

			if( oWeapon )
			{
				Magazine oMag = ( Magazine ) oWeapon.GetAttachmentByConfigTypeName( "DefaultMagazine" );

				if( oMag && oMag.IsMagazine() )
				{
					oMag.LocalSetAmmoMax();
				}
				
				Object oSupressor = ( Object ) oWeapon.GetAttachmentByConfigTypeName( "SuppressorBase" );

				if( oSupressor )
				{
					oSupressor.SetHealth( oSupressor.GetMaxHealth( "", "" ) );
				}
			}
			
		}
		
		UpdateDebugMonitor();

		UpdateAutoWalk();

		// UpdateEditor(); --- Missing / Removed ??
        
		if(m_oPlayer) {
		    m_oPlayer.StatUpdateByTime("playtime");
		    cpl.UpdateLifespan( m_oPlayer, true );
		    cpl.UpdateBloodyHandsVisibility(m_oPlayer, m_oPlayer.HasBloodyHands());

			if (m_bOldAiming) {
				m_oPlayer.OverrideShootFromCamera(true);
			}
		}
		
		for ( int i = 0; i < m_Modules.Count(); ++i) 
		{
			Module module = m_Modules.Get(i);
			
			for ( int kb = 0; kb < module.GetBindings().Count(); ++kb )
			{
				KeyMouseBinding k_m_Binding = module.GetBindings().Get(kb);
				
				if ( k_m_Binding.IsRecurring() )
				{
					if (k_m_Binding.Check()) 
					{
						int mouseButton = -1;
	
						bool hasDrag = false;
						for ( int mb = 0; mb < k_m_Binding.GetMouseBinds().Count(); ++mb) 
						{
							int mouseBind = k_m_Binding.GetMouseBinds().GetKey(mb);
							int mouseEvent = k_m_Binding.GetMouseBinds().Get(mouseBind);
							
							if ( mouseEvent == MB_EVENT_DRAG ) 
							{
								hasDrag = true;
								mouseButton = mouseBind;
							}
							else if ( mouseEvent == MB_EVENT_HOLD ) 
							{
								mouseButton = mouseBind;
							}
						}
						
						if ( mouseButton > -1 )
						{
							MouseButtonInfo info = m_MouseButtons.Get( mouseButton );
							if ( info ) 
							{
								if ( info.IsButtonDown() ) 
								{
									int time_curr = GetGame().GetTime();
									int time_hold = info.GetTimeLastPress() + HOLD_CLICK_TIME_MIN;
									
									if ( time_hold < time_curr )
									{
										if ( hasDrag ) 
										{
											GetGame().GameScript.CallFunction(module, k_m_Binding.GetCallBackFunction(), NULL, 0 );
										}
										else if ( k_m_Binding.ContainsButtonEvent( mouseButton, MB_EVENT_HOLD) )
										{
											GetGame().GameScript.CallFunction(module, k_m_Binding.GetCallBackFunction(), NULL, 0 );
										}
									}
								}
							}
						}
						
						if ( k_m_Binding.ContainsButton( MouseState.WHEEL ) )
						{
							if ( GetMouseState (MouseState.WHEEL) != 0 )
							{
								GetGame().GameScript.CallFunction(module, k_m_Binding.GetCallBackFunction(), NULL, GetMouseState( MouseState.WHEEL ) );
							}
						}
					
						if ( k_m_Binding.HasKeyEvent(KB_EVENT_HOLD) )
						{
							GetGame().GameScript.CallFunction(module, k_m_Binding.GetCallBackFunction(), NULL, 0 );
						}
					}
				}
			}
			module.onUpdate( timeslice );
		}
	}
	
	
	
	override void OnMouseButtonRelease(int button)
	{
		super.OnMouseButtonRelease(button);
	
		MouseButtonInfo button_info = GetMouseButtonInfo( button );
		if (button_info == NULL) return;
	
		int time_curr			= GetGame().GetTime();
		int time_last_press		= button_info.GetTimeLastPress();
		int time_last_release	= button_info.GetTimeLastRelease();	
		int time_delta_press	= time_curr - time_last_press;
		int time_delta_release	= time_curr - time_last_release;
	
		for ( int i = 0; i < m_Modules.Count(); ++i) 
		{
			Module module = m_Modules.Get(i);
			
			for ( int kb = 0; kb < module.GetBindings().Count(); ++kb )
			{
				KeyMouseBinding k_m_Binding = module.GetBindings().Get(kb);
				
				if ( k_m_Binding.ContainsButton( button ) ) 
				{
					if ( k_m_Binding.Check() )
					{
						if ( time_delta_release < DOUBLE_CLICK_TIME )
						{
							if ( k_m_Binding.ContainsButtonEvent( button, MB_EVENT_DOUBLECLICK ) )
							{
								GetGame().GameScript.CallFunction(module, k_m_Binding.GetCallBackFunction(), NULL, 0 );
							}
						}
						else if ( time_delta_press < CLICK_TIME )
						{
							if ( k_m_Binding.ContainsButtonEvent( button, MB_EVENT_CLICK ) )
							{
								GetGame().GameScript.CallFunction(module, k_m_Binding.GetCallBackFunction(), NULL, 0 );
							}
						} 
						else 
						{
							if ( k_m_Binding.ContainsButtonEvent( button, MB_EVENT_RELEASE ) )
							{
								GetGame().GameScript.CallFunction(module, k_m_Binding.GetCallBackFunction(), NULL, 0 );
							} 
						}
					}
				}
			}
			module.onMouseButtonRelease( button );
		}
		button_info.Release();
	}
	
	
	
	
	override void OnMouseButtonPress( int button )
	{
		super.OnMouseButtonPress( button );
		
		MouseButtonInfo button_info = GetMouseButtonInfo( button );
		if (button_info == NULL) return;
		button_info.Press(); // Update press time
		
		for ( int i = 0; i < m_Modules.Count(); ++i)
		{
			Module module = m_Modules.Get(i);

			for ( int kb = 0; kb < module.GetBindings().Count(); ++kb ) 
			{
				KeyMouseBinding k_m_Binding = module.GetBindings().Get(kb);
				
				if ( k_m_Binding.ContainsButton( button ) ) 
				{
					if ( k_m_Binding.ContainsButtonEvent( button, MB_EVENT_PRESS ) ) 
					{
						if ( k_m_Binding.Check() ) 
						{
							GetGame().GameScript.CallFunction(module, k_m_Binding.GetCallBackFunction(), NULL, 0 );
						}
					}
				}
			}
			module.onMouseButtonPress( button ); // extra utility
		}
	}
	
	
	
	
	override void OnKeyPress( int key )
	{
		super.OnKeyPress(key);

		for ( int i = 0; i < m_Modules.Count(); ++i)
		{
			Module module = m_Modules.Get(i);

			for ( int kb = 0; kb < module.GetBindings().Count(); ++kb ) 
			{
				KeyMouseBinding k_m_Binding = module.GetBindings().Get(kb);
				
				if ( k_m_Binding.ContainsKeyEvent( key, KB_EVENT_PRESS ) ) 
				{
					if ( k_m_Binding.Check() ) 
					{
						GetGame().GameScript.CallFunction(module, k_m_Binding.GetCallBackFunction(), NULL, 0 );
					}
				}
			}
			
			module.onKeyPress( key ); // extra utility
		}
		
		if( GetGame().GetUIManager().GetMenu() )
		{
			return;
		}

		// dannydog: port over old keybinds and functions to new module system
		
		if( !m_bWelcome )
		{
		    m_bWelcome = true;
            WelcomeMenu oWelcomeMenu = new WelcomeMenu;
            oWelcomeMenu.Init();
            GetGame().GetUIManager().ShowScriptedMenu( oWelcomeMenu, NULL );
		}

		switch( key )
		{
			case KeyCode.KC_LCONTROL:
			{
				m_IsCtrlHolding = true;
				break;
			}
			
			case KeyCode.KC_LWIN:
			{
				m_IsWinHolding = true;
				break;
			}
			
			case KeyCode.KC_LMENU:
			{
				m_IsLeftAltHolding = true;
				break;	
			}
			
			case KeyCode.KC_RMENU:
			{
				m_IsRightAltHolding = true;
				break;
			}
			
			case KeyCode.KC_LSHIFT:
			{
				m_IsLeftShiftHolding = true;
				break:
			}
			
			case KeyCode.KC_RSHIFT:
			{
				m_IsRightShiftHolding = true;
				break:
			}


			//Gestures [.]
			case KeyCode.KC_PERIOD:
			{
				//open gestures menu
				if ( !GetUIManager().IsMenuOpen( MENU_GESTURES ) )
				{
					GesturesMenu.OpenMenu();
				}
				
				break;
			}


			//Radial Quickbar [,]
			case KeyCode.KC_COMMA:
			{
				//open radial quickbar menu
				if ( !GetGame().GetUIManager().IsMenuOpen( MENU_RADIAL_QUICKBAR ) )
				{
					RadialQuickbarMenu.OpenMenu();
				}
				
				break;
			}


			case KeyCode.KC_T:
			{
				if ( CameraTool.Cast(GetModule(CameraTool)).IsUsingCamera() ) 
				{
			        m_oPlayer.MessageStatus( "You can not teleport while you are inside the freecam!" );

			        return;
			    }

				vector hitPos = GetCursorPos();

				float distance = vector.Distance( m_oPlayer.GetPosition(), hitPos );
				
				if ( distance < 5000 )
				{
					EntityAI oVehicle = m_oPlayer.GetDrivingVehicle();

					if( oVehicle )
					{
						m_oPlayer.MessageStatus("Get out of the vehicle first!");
					}
					else
					{
						m_oPlayer.SetPosition( hitPos );
						m_oPlayer.MessageStatus("Teleported!");
					}
				}
				else
				{
					m_oPlayer.MessageStatus( "Distance for teleportation is too far!" );
				}	

				break;
			}


			case KeyCode.KC_O:
			{
				if( CTRL() )
				{
					GetGame().CreateObject( "Animal_CanisLupus_Grey", GetCursorPos(), false, true );
				}
				else if( SHIFT() )
				{
					GetGame().CreateObject( GetRandomChildFromBaseClass( "cfgVehicles", "AnimalBase" ), GetCursorPos(), false, true );
				}
				else
				{
					GetGame().CreateObject( WorkingZombieClasses().GetRandomElement(), GetCursorPos(), false, true );
				}
				
				break;
			}


			case KeyCode.KC_R:
			{
				EntityAI oWeapon = m_oPlayer.GetHumanInventory().GetEntityInHands();
				
				if( oWeapon )
				{
					Magazine oMag = ( Magazine ) oWeapon.GetAttachmentByConfigTypeName( "DefaultMagazine" );
					
					if( oMag && oMag.IsMagazine() )
					{
						oMag.LocalSetAmmoMax();
					}					
					
					Object oSupressor = ( Object ) oWeapon.GetAttachmentByConfigTypeName( "SuppressorBase" );
					
					if( oSupressor )
					{
						oSupressor.SetHealth( oSupressor.GetMaxHealth( "", "" ) );
					}
				}

				break;
			}


			case KeyCode.KC_P:
			{
				Print( m_oPlayer.GetPosition().ToString() );
				Print( m_oPlayer.GetDirection().ToString() );
				
				Print( "POS X:" + m_oPlayer.GetPosition()[0] + " Y:" + m_oPlayer.GetPosition()[2] + " Z:" + m_oPlayer.GetPosition()[1] );
				m_oPlayer.MessageStatus( "POS X:" + m_oPlayer.GetPosition()[0] + " Y:" + m_oPlayer.GetPosition()[2] + " Z:" + m_oPlayer.GetPosition()[1] );

				m_oPlayer.MessageStatus( "Player position and direction vector were written to the game logs too." );
				
				break;
			}


			case KeyCode.KC_B:
			{
                if( m_bDebugMonitor )
                {
                    m_debugMonitorPatched.Hide();
                    m_bDebugMonitor = false;
                }
                else
                {
                    m_debugMonitorPatched.Show();
                    m_bDebugMonitor = true;
                }

				break;
			}


            case KeyCode.KC_X:
            {
                if( m_nAutoWalkMode && !SHIFT() )
                {
                    m_nAutoWalkMode = 0;
                    m_oPlayer.GetInputController().OverrideMovementSpeed( false, 0 );
                    m_oPlayer.GetInputController().OverrideMovementAngle( false, 0 );
                }
                else
                {
                    if( SHIFT() )
                    {
                        m_nAutoWalkMode = 2;
                    }
                    else
                    {
                        m_nAutoWalkMode = 1;
                    }
                }

                break;
            }


			case KeyCode.KC_K:
			{
				GetGame().GetCallQueue( CALL_CATEGORY_GUI ).Call( GetGame().RestartMission );
				break;
			}


			case KeyCode.KC_N:
			{
				TStringArray attArr = {
				"CivSedanWheel","CivSedanWheel","CivSedanWheel","CivSedanWheel",
				"CarBattery","CarRadiator","EngineBelt","SparkPlug","CivSedanHood",
				"CivSedanTrunk","CivSedanDoors_Driver","CivSedanDoors_CoDriver",
				"CivSedanDoors_BackLeft","CivSedanDoors_BackRight",
				}; 
				
				EntityAI oCar = EntityAI.Cast( GetGame().CreateObject( "CivilianSedan", GetCursorPos(), false, true ) );
				
				for (int j = 0; j < attArr.Count(); j++) { oCar.GetInventory().CreateAttachment( attArr.Get(j) ); }
				
				oCar.SetAllowDamage( false );
				
				break;
			}

			case KeyCode.KC_END:
			{
				if ( m_bGodMode )
				{
					m_oPlayer.MessageStatus( "God mode disabled." );
					m_oPlayer.SetAllowDamage( true );
					m_bGodMode = false;
				}
				else
				{
					m_oPlayer.MessageStatus( "God mode enabled." );
					m_oPlayer.SetAllowDamage( false );
					m_bGodMode = true;
				}

				break;
			}		

			case KeyCode.KC_L:
			{
				if ( m_bOldAiming )
				{
					m_oPlayer.OverrideShootFromCamera(false);
					m_oPlayer.MessageStatus( "Old Aiming System disabled." );
					m_bOldAiming = false;
				}
				else
				{
					m_oPlayer.OverrideShootFromCamera(true);
					m_oPlayer.MessageStatus( "Old Aiming System enabled." );
					m_bOldAiming = true;
				}

				break;
			}	


			case KeyCode.KC_DELETE:
			{

                UIScriptedMenu oMenu = NULL;

				if( CTRL() )
				{
				    oMenu = new PositionMenu;
				}
				else if( SHIFT() )
				{
                    oMenu = new WeatherMenu;
				}
				else
				{
				    oMenu = new ObjectMenu;
				}

				oMenu.Init();
                GetGame().GetUIManager().ShowScriptedMenu( oMenu, NULL );

				break;
			}


			case KeyCode.KC_HOME:
			{
				DamageSystem.ExplosionDamage( NULL, NULL, "Dummy_Heavy", GetCursorPos() );
				Particle.Play( ParticleList.EXPLOSION_TEST, GetCursorPos() );
				break;
			}
		}	
	}
	
	
	
	override void OnKeyRelease(int key)
	{
		super.OnKeyRelease(key);
				
		for ( int i = 0; i < m_Modules.Count(); ++i)
		{
			
			Module module = m_Modules.Get(i);

			for ( int kb = 0; kb < module.GetBindings().Count(); ++kb ) 
			{
				KeyMouseBinding k_m_Binding = module.GetBindings().Get(kb);
				if ( k_m_Binding.ContainsKeyEvent( key, KB_EVENT_RELEASE ) ) 
				{
					if ( k_m_Binding.Check() ) 
					{
						GetGame().GameScript.CallFunction(module, k_m_Binding.GetCallBackFunction(), NULL, 0 );
					}
				} 
			}
			module.onKeyRelease( key ); // extra utility
		}
				
		switch( key )
		{
			case KeyCode.KC_LCONTROL:
			{
				m_IsCtrlHolding = false;
				break;
			}
			
			
			case KeyCode.KC_LWIN:
			{
				m_IsWinHolding = false;
				break;
			}
			
			
			case KeyCode.KC_LMENU:
			{
				m_IsLeftAltHolding = false;
				break;
			}
						
			
			case KeyCode.KC_RMENU:
			{
				m_IsRightAltHolding = false;
				break;
			}
			
			case KeyCode.KC_LSHIFT:
			{
				m_IsLeftShiftHolding = false;
				break:
			}
			
			case KeyCode.KC_RSHIFT:
			{
				m_IsRightShiftHolding = false;
				break:
			}
		}
		//Gestures [.]
		if ( key == KeyCode.KC_PERIOD )
		{
			if ( GetUIManager().IsMenuOpen( MENU_GESTURES ) )
			{
				GesturesMenu.CloseMenu();
			}
		}
		//Radial Quickbar [,]
		if ( key == KeyCode.KC_COMMA )
		{
			if ( GetGame().GetUIManager().IsMenuOpen( MENU_RADIAL_QUICKBAR ) )
			{
				RadialQuickbarMenu.CloseMenu();
			}
		}
	}

	
	
	
    override void CreateDebugMonitor()
    {
        if (!m_debugMonitorPatched)
        {
            m_debugMonitorPatched = new DebugMonitorPatched();
            m_debugMonitorPatched.Init();
        }
    }

	
	
	
    override void UpdateDebugMonitor()
    {
        if (!m_debugMonitorPatched) return;

        if (m_oPlayer)
        {
            m_debugMonitorPatched.SetHealth( m_oPlayer.GetHealth( "","" ) );
            m_debugMonitorPatched.SetBlood(  m_oPlayer.GetHealth( "","Blood" ) );
            m_debugMonitorPatched.SetLastDamage( "" );
            m_debugMonitorPatched.SetPosition( m_oPlayer.GetPosition() );
        }
    }

	
	
	
    void UpdateAutoWalk()
    {
        if( m_nAutoWalkMode )
        {
            //m_oPlayer.GetStaminaHandler().GetStaminaNormalized()
            if( ( m_oPlayer.GetInputController().LimitsIsSprintDisabled() ) || ( m_nAutoWalkMode == 1 ) )
            {
                m_oPlayer.GetInputController().OverrideMovementSpeed( true, 2 );
            }
            else
            {
                m_oPlayer.GetInputController().OverrideMovementSpeed( true, 3 );
            }

            m_oPlayer.GetInputController().OverrideMovementAngle( true, 1 );
        }
    }

	
	
	
    void SetupWeather()
    {
        //Offical DayZ SA weather code
        Weather weather = g_Game.GetWeather();

        weather.GetOvercast().SetLimits( 0.0 , 2.0 );
        weather.GetRain().SetLimits( 0.0 , 2.0 );
        weather.GetFog().SetLimits( 0.0 , 2.0 );

        weather.GetOvercast().SetForecastChangeLimits( 0.0, 0.0 );
        weather.GetRain().SetForecastChangeLimits( 0.0, 0.0 );
        weather.GetFog().SetForecastChangeLimits( 0.0, 0.0 );

        weather.GetOvercast().SetForecastTimeLimits( 1800 , 1800 );
        weather.GetRain().SetForecastTimeLimits( 600 , 600 );
        weather.GetFog().SetForecastTimeLimits( 600 , 600 );

        weather.GetOvercast().Set( 0.0, 0, 0 );
        weather.GetRain().Set( 0.0, 0, 0 );
        weather.GetFog().Set( 0.0, 0, 0 );

        weather.SetWindMaximumSpeed( 50 );
        weather.SetWindFunctionParams( 0, 0, 1 );
    }

	vector GetCursorPos()
	{
		vector rayStart = GetGame().GetCurrentCameraPosition();
		vector rayEnd = rayStart + GetGame().GetCurrentCameraDirection() * 10000;
		vector hitPos;
		vector hitNormal;
		int hitComponentIndex;
		DayZPhysics.RaycastRV(rayStart, rayEnd, hitPos, hitNormal, hitComponentIndex, NULL, NULL, m_oPlayer);

		return hitPos;
	}

	string GetRandomChildFromBaseClass( string strConfigName, string strBaseClass )
	{
		string child_name = "";
		int count = GetGame().ConfigGetChildrenCount ( strConfigName );
		array<string> class_names = new array<string>;

		for (int p = 0; p < count; p++)
		{
			GetGame().ConfigGetChildName ( strConfigName, p, child_name );

			if ( GetGame().IsKindOf(child_name, strBaseClass ) && ( child_name != strBaseClass ) )
			{
				class_names.Insert(child_name);
			}
		}

		return class_names.GetRandomElement();
	}

	bool CTRL() return m_IsCtrlHolding;
	bool WIN() return m_IsWinHolding;
	bool SHIFT() return ( m_IsLeftShiftHolding || m_IsRightShiftHolding );
	bool ALT() return ( m_IsLeftAltHolding || m_IsRightAltHolding );


	void SpawnPlayer()
    {
        TVectorArray positions = { "15135.1 0 13901.1", "15017.8 0 13892.4", "14887.1 0 14547.9", "14749.7 0 13248.7",
                                   "14697.6 0 13418.4", "14537.3 0 14755.7", "14415.3 0 14025.2", "14338.0 0 12859.5",
                                   "14263.8 0 12748.7", "14172.2 0 12304.9", "14071.4 0 12033.3", "14054.9 0 11341.3",
                                   "14017.8 0 2959.1", "13905.5 0 12489.7", "13852.4 0 11686.0", "13846.6 0 12050.0",
                                   "13676.0 0 12262.1", "13617.4 0 12759.8", "13610.1 0 11223.6", "13594.3 0 4064.0",
                                   "13587.8 0 6026.5", "13571.1 0 3056.8", "13552.6 0 4653.7", "13529.9 0 3968.3",
                                   "13520.8 0 4223.7", "13504.0 0 5004.5", "13476.7 0 6136.3", "13441.6 0 5262.2",
                                   "13426.6 0 5747.3", "13416.8 0 11840.4", "13400.8 0 4120.7", "13395.8 0 5902.8",
                                   "13385.0 0 3946.6", "13374.4 0 6454.3", "13367.1 0 10837.1", "13366.3 0 4906.0",
                                   "13337.1 0 5120.8", "13326.7 0 5489.1", "13312.7 0 6771.1", "13288.7 0 11415.1",
                                   "13261.6 0 11785.2", "13171.6 0 6534.8", "13159.8 0 5401.7", "13155.2 0 5475.2",
                                   "13084.9 0 7938.6", "13056.8 0 4848.5", "13048.1 0 8357.6", "13048.1 0 3867.7",
                                   "12991.7 0 7287.1", "12983.0 0 5539.1", "12978.9 0 9727.8", "12950.2 0 5226.7",
                                   "12942.1 0 8393.1", "12891.5 0 3673.9", "12628.7 0 10495.2", "12574.3 0 3592.8",
                                   "12566.3 0 6682.6", "12465.2 0 8009.0", "12354.5 0 3480.0", "13262.8 0 7225.8" };

        m_oPlayer = PlayerBase.Cast( GetGame().CreatePlayer( NULL, GetGame().CreateRandomPlayer(), positions.GetRandomElement(), 0, "NONE") );

        GetGame().SelectPlayer( NULL, m_oPlayer );

        EntityAI item = m_oPlayer.GetInventory().CreateInInventory( "AviatorGlasses" );

        item = m_oPlayer.GetInventory().CreateInInventory( "MilitaryBeret_UN" );

        item = m_oPlayer.GetInventory().CreateInInventory( "M65Jacket_Black" );

        item = m_oPlayer.GetInventory().CreateInInventory( "PlateCarrierHolster" );

        item = m_oPlayer.GetInventory().CreateInInventory( "TacticalGloves_Black" );

        item = m_oPlayer.GetInventory().CreateInInventory( "HunterPants_Autumn" );

        item = m_oPlayer.GetInventory().CreateInInventory( "MilitaryBoots_Black" );

        item = m_oPlayer.GetInventory().CreateInInventory( "AliceBag_Camo" );

        item = m_oPlayer.GetInventory().CreateInInventory( "M4A1_Black" );
        item.GetInventory().CreateAttachment( "M4_Suppressor" );
        item.GetInventory().CreateAttachment( "M4_RISHndgrd_Black" );
        item.GetInventory().CreateAttachment( "M4_MPBttstck_Black" );
        item.GetInventory().CreateAttachment( "ACOGOptic" );

        auto oMag = m_oPlayer.GetInventory().CreateInInventory( "Mag_STANAGCoupled_30Rnd" );
        m_oPlayer.GetInventory().CreateInInventory( "Mag_STANAGCoupled_30Rnd" );
        m_oPlayer.GetInventory().CreateInInventory( "Mag_STANAGCoupled_30Rnd" );

        m_oPlayer.LocalTakeEntityToHands( item );

        m_oPlayer.SetQuickBarEntityShortcut( item, 0, true );

        m_oPlayer.GetWeaponManager().AttachMagazine( oMag );


    }


	void InitHive()
	{
		if (DISABLE_HIVE) return;
	
		Hive oHive = GetHive();
		
		if( !oHive )
		{
			oHive = CreateHive();
		}
		if( oHive )
		{
			oHive.InitOffline();
		}
	}


	TStringArray WorkingZombieClasses()
	{
		return {
		"ZmbM_HermitSkinny_Base","ZmbM_HermitSkinny_Beige","ZmbM_HermitSkinny_Black","ZmbM_HermitSkinny_Green",
		"ZmbM_HermitSkinny_Red","ZmbM_FarmerFat_Base","ZmbM_FarmerFat_Beige","ZmbM_FarmerFat_Blue","ZmbM_FarmerFat_Brown",
		"ZmbM_FarmerFat_Green","ZmbF_CitizenANormal_Base","ZmbF_CitizenANormal_Beige","ZmbF_CitizenANormal_Brown",
		"ZmbF_CitizenANormal_Blue","ZmbM_CitizenASkinny_Base","ZmbM_CitizenASkinny_Blue","ZmbM_CitizenASkinny_Brown",
		"ZmbM_CitizenASkinny_Grey","ZmbM_CitizenASkinny_Red","ZmbM_CitizenBFat_Base","ZmbM_CitizenBFat_Blue","ZmbM_CitizenBFat_Red",
		"ZmbM_CitizenBFat_Green","ZmbF_CitizenBSkinny_Base","ZmbF_CitizenBSkinny","ZmbM_PrisonerSkinny_Base","ZmbM_PrisonerSkinny",
		"ZmbM_FirefighterNormal_Base","ZmbM_FirefighterNormal","ZmbM_FishermanOld_Base","ZmbM_FishermanOld_Blue","ZmbM_FishermanOld_Green",
		"ZmbM_FishermanOld_Grey","ZmbM_FishermanOld_Red","ZmbM_JournalistSkinny_Base","ZmbM_JournalistSkinny","ZmbF_JournalistNormal_Base",
		"ZmbF_JournalistNormal_Blue","ZmbF_JournalistNormal_Green","ZmbF_JournalistNormal_Red","ZmbF_JournalistNormal_White",
		"ZmbM_ParamedicNormal_Base","ZmbM_ParamedicNormal_Blue","ZmbM_ParamedicNormal_Green","ZmbM_ParamedicNormal_Red",
		"ZmbM_ParamedicNormal_Black","ZmbF_ParamedicNormal_Base","ZmbF_ParamedicNormal_Blue","ZmbF_ParamedicNormal_Green",
		"ZmbF_ParamedicNormal_Red","ZmbM_HikerSkinny_Base","ZmbM_HikerSkinny_Blue","ZmbM_HikerSkinny_Green","ZmbM_HikerSkinny_Yellow",
		"ZmbF_HikerSkinny_Base","ZmbF_HikerSkinny_Blue","ZmbF_HikerSkinny_Grey","ZmbF_HikerSkinny_Green","ZmbF_HikerSkinny_Red",
		"ZmbM_HunterOld_Base","ZmbM_HunterOld_Autumn","ZmbM_HunterOld_Spring","ZmbM_HunterOld_Summer","ZmbM_HunterOld_Winter",
		"ZmbF_SurvivorNormal_Base","ZmbF_SurvivorNormal_Blue","ZmbF_SurvivorNormal_Orange","ZmbF_SurvivorNormal_Red",
		"ZmbF_SurvivorNormal_White","ZmbM_SurvivorDean_Base","ZmbM_SurvivorDean_Black","ZmbM_SurvivorDean_Blue","ZmbM_SurvivorDean_Grey",
		"ZmbM_PolicemanFat_Base","ZmbM_PolicemanFat","ZmbF_PoliceWomanNormal_Base","ZmbF_PoliceWomanNormal","ZmbM_PolicemanSpecForce_Base",
		"ZmbM_PolicemanSpecForce","ZmbM_SoldierNormal_Base","ZmbM_SoldierNormal","ZmbM_usSoldier_normal_Base",
		"ZmbM_usSoldier_normal_Woodland","ZmbM_usSoldier_normal_Desert","ZmbM_CommercialPilotOld_Base","ZmbM_CommercialPilotOld_Blue",
		"ZmbM_CommercialPilotOld_Olive","ZmbM_CommercialPilotOld_Brown","ZmbM_CommercialPilotOld_Grey","ZmbM_PatrolNormal_Base",
		"ZmbM_PatrolNormal_PautRev","ZmbM_PatrolNormal_Autumn","ZmbM_PatrolNormal_Flat","ZmbM_PatrolNormal_Summer","ZmbM_JoggerSkinny_Base",
		"ZmbM_JoggerSkinny_Blue","ZmbM_JoggerSkinny_Green","ZmbM_JoggerSkinny_Red","ZmbF_JoggerSkinny_Base","ZmbF_JoggerSkinny_Blue",
		"ZmbF_JoggerSkinny_Brown","ZmbF_JoggerSkinny_Green","ZmbF_JoggerSkinny_Red","ZmbM_MotobikerFat_Base","ZmbM_MotobikerFat_Beige",
		"ZmbM_MotobikerFat_Black","ZmbM_MotobikerFat_Blue","ZmbM_VillagerOld_Base","ZmbM_VillagerOld_Blue","ZmbM_VillagerOld_Green",
		"ZmbM_VillagerOld_White","ZmbM_SkaterYoung_Base","ZmbM_SkaterYoung_Blue","ZmbM_SkaterYoung_Brown","ZmbM_SkaterYoung_Green",
		"ZmbM_SkaterYoung_Grey","ZmbF_SkaterYoung_Base","ZmbF_SkaterYoung_Brown","ZmbF_SkaterYoung_Striped","ZmbF_SkaterYoung_Violet",
		"ZmbF_DoctorSkinny_Base","ZmbF_DoctorSkinny","ZmbF_BlueCollarFat_Base","ZmbF_BlueCollarFat_Blue","ZmbF_BlueCollarFat_Green",
		"ZmbF_BlueCollarFat_Red","ZmbF_BlueCollarFat_White","ZmbF_MechanicNormal_Base","ZmbF_MechanicNormal_Beige","ZmbF_MechanicNormal_Green",
		"ZmbF_MechanicNormal_Grey","ZmbF_MechanicNormal_Orange","ZmbM_MechanicSkinny_Base","ZmbM_MechanicSkinny_Blue","ZmbM_MechanicSkinny_Grey",
		"ZmbM_MechanicSkinny_Green","ZmbM_MechanicSkinny_Red","ZmbM_ConstrWorkerNormal_Base","ZmbM_ConstrWorkerNormal_Beige",
		"ZmbM_ConstrWorkerNormal_Black","ZmbM_ConstrWorkerNormal_Green","ZmbM_ConstrWorkerNormal_Grey","ZmbM_HeavyIndustryWorker_Base",
		"ZmbM_HeavyIndustryWorker","ZmbM_OffshoreWorker_Base","ZmbM_OffshoreWorker_Green","ZmbM_OffshoreWorker_Orange","ZmbM_OffshoreWorker_Red",
		"ZmbM_OffshoreWorker_Yellow","ZmbF_NurseFat_Base","ZmbF_NurseFat","ZmbM_HandymanNormal_Base","ZmbM_HandymanNormal_Beige",
		"ZmbM_HandymanNormal_Blue","ZmbM_HandymanNormal_Green","ZmbM_HandymanNormal_Grey","ZmbM_HandymanNormal_White","ZmbM_DoctorFat_Base",
		"ZmbM_DoctorFat","ZmbM_Jacket_Base","ZmbM_Jacket_beige","ZmbM_Jacket_black","ZmbM_Jacket_blue","ZmbM_Jacket_bluechecks",
		"ZmbM_Jacket_brown","ZmbM_Jacket_greenchecks","ZmbM_Jacket_grey","ZmbM_Jacket_khaki","ZmbM_Jacket_magenta","ZmbM_Jacket_stripes",
		"ZmbF_PatientOld_Base","ZmbF_PatientOld","ZmbM_PatientSkinny_Base","ZmbM_PatientSkinny","ZmbF_ShortSkirt_Base","ZmbF_ShortSkirt_beige",
		"ZmbF_ShortSkirt_black","ZmbF_ShortSkirt_brown","ZmbF_ShortSkirt_green","ZmbF_ShortSkirt_grey","ZmbF_ShortSkirt_checks",
		"ZmbF_ShortSkirt_red","ZmbF_ShortSkirt_stripes","ZmbF_ShortSkirt_white","ZmbF_ShortSkirt_yellow","ZmbF_VillagerOld_Base",
		"ZmbF_VillagerOld_Blue","ZmbF_VillagerOld_Green","ZmbF_VillagerOld_Red","ZmbF_VillagerOld_White","ZmbM_Soldier","ZmbM_SoldierAlice",
		"ZmbM_SoldierHelmet","ZmbM_SoldierVest","ZmbM_SoldierAliceHelmet","ZmbM_SoldierVestHelmet","ZmbF_MilkMaidOld_Base",
		"ZmbF_MilkMaidOld_Beige","ZmbF_MilkMaidOld_Black","ZmbF_MilkMaidOld_Green","ZmbF_MilkMaidOld_Grey","ZmbM_priestPopSkinny_Base",
		"ZmbM_priestPopSkinny","ZmbM_ClerkFat_Base","ZmbM_ClerkFat_Brown","ZmbM_ClerkFat_Grey","ZmbM_ClerkFat_Khaki","ZmbM_ClerkFat_White",
		"ZmbF_Clerk_Normal_Base","ZmbF_Clerk_Normal_Blue","ZmbF_Clerk_Normal_White","ZmbF_Clerk_Normal_Green","ZmbF_Clerk_Normal_Red",
		};
	}
}
