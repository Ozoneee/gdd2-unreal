// Fill out your copyright notice in the Description page of Project Settings.

#include "Ast3roids.h"
#include "NetworkPlayerController.h"

#include "SocketSubsystem.h"
#include "IPv4SubnetMask.h"
#include "IPv4Address.h"
#include "IPv4Endpoint.h"
#include "TcpSocketBuilder.h"
#include "UnrealMathUtility.h"

// Sets default values
ANetworkPlayerController::ANetworkPlayerController(const FObjectInitializer& ObjectInitializer)
	: APlayerController(ObjectInitializer)
	, Socket(nullptr)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}



ANetworkPlayerController::~ANetworkPlayerController()
{
	if (Connection != nullptr)
	{
		Connection->Close();

		delete Connection;
	}

	if (Socket != nullptr)
	{
		Socket->Close();

		delete Socket;
	}
}

// Called when the game starts or when spawned
void ANetworkPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FString address = TEXT("0.0.0.0");
	int32 port = 19834;

	bool valid = true;

	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	addr->SetIp(*address, valid);
	addr->SetPort(port);

	FIPv4Endpoint Endpoint(addr);

	Socket = FTcpSocketBuilder(TEXT("default")).AsReusable().BoundToEndpoint(Endpoint).Listening(8);

	int32 new_size;
	Socket->SetReceiveBufferSize(2 << 20, new_size);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connection ~> %d"), Socket != nullptr));

	GetWorldTimerManager().SetTimer(this,
		&ANetworkPlayerController::TCPConnectionListener, 0.01f, true);//*/
	
}

// Called every frame
void ANetworkPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANetworkPlayerController::TCPConnectionListener()
{
	if (!Socket) return;

	//Remote address
	RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool Pending;

	// handle incoming connections
	if (Socket->HasPendingConnection(Pending) && Pending)
	{
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//Already have a Connection? destroy previous
		if (Connection)
		{
			Connection->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Connection);
		}
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		//New Connection receive!
		Connection = Socket->Accept(*RemoteAddress, TEXT("connection"));

		if (Connection != NULL)
		{
			//can thread this too
			GetWorldTimerManager().SetTimer(this,
				&ANetworkPlayerController::TCPSocketListener, 0.01, true);
		}
	}
}

void ANetworkPlayerController::TCPSocketListener()
{
	if (!Connection) return;

	TArray<uint8> ReceivedData;

	uint32 Size;
	while (Connection->HasPendingData(Size))
	{
		ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));

		int32 Read = 0;
		Connection->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);
	}

	if (ReceivedData.Num() <= 0)
	{
		//No Data Received
		return;
	}

	int length = ReceivedData.Num() / sizeof(float);

	const float* angles = reinterpret_cast<const float*>(ReceivedData.GetData());

	float pitch = -FMath::RadiansToDegrees(angles[length - 2]);
	float yaw = FMath::RadiansToDegrees(angles[length - 3]);
	float roll = -FMath::RadiansToDegrees(angles[length - 1]);

	FQuat Pitch = FQuat(FRotator(pitch, 0, 0));
	FQuat Yaw = FQuat(FRotator(0, yaw, 0));
	FQuat Roll = FQuat(FRotator(0, 0, roll));
	
	FQuat Rotation = Yaw * Pitch * Roll;

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%f %f %f"), pitch, yaw, roll));
	SetControlRotation(FRotator(Rotation));
}


